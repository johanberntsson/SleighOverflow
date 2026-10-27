// Stranded, by Mattias Gustavsson: src/stranded.c, ported to the MEGA65.
//
// Everything above the framework line is the original's, and kept as close to
// it as the machine allows; every change is marked PORT. The framework is
// src/dos.c. What changed, in short:
//
//   - the bitmaps are in attic RAM before this starts (src/gfx.h), and the
//     beach's background, treeline and props are one picture, WORLD, so their
//     sprites are kept for their positions and draw nothing;
//   - drawsprites starts at the topmost picture that covers the whole screen,
//     and draws text from labels rendered offline (tools/labels.py);
//   - the easing functions are tables (tools/convtables.py), and the two
//     animation counters are integers;
//   - it runs at 50 frames a second, PAL, where the original ran at 60.
//
// See end of file for license for this code
//    /Mattias Gustavsson

#include <stdlib.h>
#include <string.h>
#include "dos.h"
#include "gfx.h"
#include "tables.h"

// PORT: the bitmap enum is generated from the assets (build/gfx_ids.h).
// These were separate bitmaps in the original and are now part of WORLD,
// which is drawn in BACKGROUND's place; their sprites stay, because the game
// measures distances from them, and draw nothing.
#define BACKGROUND WORLD
#define TREELINE   INVALID
#define SACK_SPR  INVALID
#define HUT1       INVALID
#define HUT2       INVALID
#define TREE2      INVALID
#define PALMTREE   INVALID
#define CHAIR      INVALID


struct {
    int x;
    int y;
    int origin_x;
    int origin_y;
    int visible;
    int cel;
    enum bitmap_t bitmap;
    char const* label_text;
    int label_color;
    int label_wrap;
    // PORT: the label bitmap last found for this text, colour and wrap.
    char const* found_text;
    int found_color;
    int found_wrap;
    int found_id;
} sprites[ 64 ];

int spr_index = 0;

int sprite( int spr, int x, int y, enum bitmap_t bitmap ) {
    sprites[ spr ].x = x;
    sprites[ spr ].y = y;
    sprites[ spr ].visible = 1;
    sprites[ spr ].bitmap = bitmap;
    sprites[ spr ].label_text = NULL;
    sprites[ spr ].label_color = 0;
    sprites[ spr ].label_wrap = 0;
    return spr;
};

int label( int spr, int x, int y, char const* text, int color ) {
    sprites[ spr ].x = x;
    sprites[ spr ].y = y;
    sprites[ spr ].visible = 1;
    sprites[ spr ].bitmap = INVALID;
    sprites[ spr ].label_text = text;
    sprites[ spr ].label_color = color;
    sprites[ spr ].label_wrap = 0;
    return spr;
};

void sprite_hide( int spr ) {
    sprites[ spr ].visible = 0;
}

void sprite_show( int spr ) {
    sprites[ spr ].visible = 1;
}

int sprite_x( int spr ) {
    return sprites[ spr ].x;
}

int sprite_y( int spr ) {
    return sprites[ spr ].y;
}

void sprite_pos( int spr, int x, int y ) {
    sprites[ spr ].x = x;
    sprites[ spr ].y = y;
}

void sprite_cel( int spr, int cel ) {
    sprites[ spr ].cel = cel;
}

void sprite_origin( int spr, int x, int y ) {
    sprites[ spr ].origin_x = x;
    sprites[ spr ].origin_y = y;
}

void sprite_bitmap( int spr, enum bitmap_t bitmap ) {
    sprites[ spr ].bitmap = bitmap;
}

void label_color( int spr, int color ) {
    sprites[ spr ].label_color = color;
}

void label_wrap( int spr, int wrap ) {
    sprites[ spr ].label_wrap = wrap;
}

void label_text( int spr, char const* text ) {
    sprites[ spr ].label_text = text;
}


// PORT: which TEXT_n bitmap is this label? Looked up by the string, once
// each time a sprite's text changes: the strings are the original's own.
static int label_bitmap( int i ) {
    if( sprites[ i ].found_text != sprites[ i ].label_text
     || sprites[ i ].found_color != sprites[ i ].label_color
     || sprites[ i ].found_wrap != sprites[ i ].label_wrap ) {
        int t;
        sprites[ i ].found_text = sprites[ i ].label_text;
        sprites[ i ].found_color = sprites[ i ].label_color;
        sprites[ i ].found_wrap = sprites[ i ].label_wrap;
        sprites[ i ].found_id = INVALID;
        for( t = 0; t < GFX_TEXTS; ++t ) {
            if( gfx_texts[ t ].color == sprites[ i ].label_color
             && gfx_texts[ t ].wrap == sprites[ i ].label_wrap
             && strcmp( gfx_texts[ t ].text, sprites[ i ].label_text ) == 0 ) {
                sprites[ i ].found_id = TEXT_0 + t;
                break;
            }
        }
    }
    return sprites[ i ].found_id;
}


// PORT: does sprite i, a bitmap, hide the whole screen?
static int covers_screen( int i ) {
    struct bitmap const* b = &bitmaps[ sprites[ i ].bitmap ];
    int x0 = sprites[ i ].x - sprites[ i ].origin_x + b->x;
    int y0 = sprites[ i ].y - sprites[ i ].origin_y + b->y - YCROP;
    return b->opaque && x0 <= 0 && x0 + (int) b->w >= 320
        && y0 <= 0 && y0 + (int) b->h >= 200;
}


void drawsprites( void ) {
    // PORT: start at the topmost visible picture that covers the screen, and
    // draw it from the backdrop cache in chip RAM. Everything under it would
    // be drawn over anyway, and drawing it is not free: inside the hut, the
    // beach would be read out of attic RAM a whole screenful every frame.
    int first = 0;
    for( int i = spr_index - 1; i >= 0; --i ) {
        if( sprites[ i ].visible && sprites[ i ].bitmap != INVALID
         && covers_screen( i ) ) {
            first = i;
            break;
        }
    }
    for( int i = first; i < spr_index; ++i ) {
        if( !sprites[ i ].visible ) continue;
        enum bitmap_t bitmap = sprites[ i ].bitmap;
        char const* label = sprites[ i ].label_text;
        int xp = sprites[ i ].x - sprites[ i ].origin_x;
        int yp = sprites[ i ].y - sprites[ i ].origin_y;
        if( bitmap != INVALID ) {
            if( i == first && covers_screen( i ) )
                gfx_backdrop( bitmap, xp );
            else
                gfx_blit( bitmap, sprites[ i ].cel, xp, yp );
        } else if( label && *label ) {
            // PORT: the outline, the shadow and the text in one bitmap.
            int id = label_bitmap( i );
            if( id != INVALID ) gfx_blit( id, 0, xp, yp );
        }
    }
}


struct dialog_t {
    char const* actor;
    char const* line;
};


struct dialog_t dlg_lets_go_home[] = {
    { "player_sitting", "Hmmm... maybe three and a half years on this island is enough..." },
    { "player_sitting", "I've enjoyed being stranded here, but now I think I'd like to go home." },
    { "player_sitting", "Last time I checked, the radio was working fine. I think I'll call for help now." },
    { "#SetFlag","may_stand" },
    { NULL, NULL }
};

struct dialog_t dlg_sack_first[] = {
    { "player_sack", "Oh no! There are parts missing!" },
    { "player_sack", "Someone have grabbed the ANTENNA, the BATTERY, and the BULB." },
    { "player_sack", "I *have* to find them!" },
    { "#SetFlag", "looked_at_sack" },
    { NULL, NULL }
};

struct dialog_t dlg_sack_none[] = {
    { "player_sack", "It is missing the ANTENNA, the BATTERY, and the BULB." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_bulb[] = {
    { "player_sack", "It is missing the ANTENNA and the BATTERY." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_dish[] = {
    { "player_sack", "It is missing the BULB and the BATTERY." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_battery[] = {
    { "player_sack", "It is missing the ANTENNA and the BULB." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_bulb_dish[] = {
    { "player_sack", "It is missing the BATTERY." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_battery_dish[] = {
    { "player_sack", "It is missing the BULB." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_bulb_battery[] = {
    { "player_sack", "It is missing the ANTENNA." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_all[] = {
    { "player_sack", "The radio is complete! Now I can go home." },
    { "player_sack", "MAYDAY! MAYDAY! I AM STRANDED ON AN ISLAND. SEND HELP!" },
    { "#SetFlag", "rescued" },
    { NULL, NULL }
};

struct dialog_t dlg_hut_tv[] = {
    { "player_hut", "Hey, that's my antenna!" },
    { "tv_guy", "Huh, what? No, it's my antenna now. I found it." },
    { "tv_guy", "I need it for my TV. To watch my shows." },
    { "player_hut", "But *I* need it for my radio. So I can call for help and get home." },
    { "tv_guy", "Uh... I *am* very tired, I need to sleep. But I can't stop watching." },
    { "player_hut", "You can have the antenna back later, I only need it to make the transmission." },
    { "tv_guy", "Ok, go ahead then." },
    { "#ClearFlag", "at_hut_tv" },
    { NULL, NULL }
};

struct dialog_t dlg_hut_tv_sleep[] = {
    { "player_hut", "Looks like he's sleeping. I better not disturb him." },
    { NULL, NULL }
};

struct dialog_t dlg_hut_book[] = {
    { "player_hut", "That's my bulb! For my radio." },
    { "book_guy", "Oh, yes yes, quite. I need it so I can read my books." },
    { "player_hut", "But *I* need it for my radio. So I can call for help and get home." },
    { "book_guy", "I see, I see. Well I am really rather sleepy. But I can not put this book down. Quite the page-turner it is." },
    { "player_hut", "You can have the bulb back later, I only need it to make the transmission." },
    { "book_guy", "Ah, very well, very well." },
    { "#ClearFlag", "at_hut_bulb" },
    { NULL, NULL }
};

struct dialog_t dlg_hut_book_sleep[] = {
    { "player_hut", "Looks like he's sleeping. I better not disturb him." },
    { NULL, NULL }
};

struct dialog_t dlg_sealion[] = {
    { "sealion", "Waddya want!?" },
    { "player", "Uh, is that my battery you have there?" },
    { "sealion", "Don't know! Don't care!" },
    { "player", "But you're not even using it!" },
    { "sealion", "Sure I do! I lick it." },
    { "player", "You *lick* it?" },
    { "sealion", "Makes my tongue tingle." },
    { "player", "But *I* need it for my radio. So I can call for help and get home." },
    { "player", "What are you even doing here? Aren't you supposed to be on the south pole or something?" },
    { "sealion", "Vacation." },
    { "sealion", "It's about time I got back anyway." },
    { "sealion", "Don't touch my battery while I'm gone!" },
    { "#SetFlag", "sealion_running" },
    { "player", "Yeah, right." },
    { "#SetFlag", "sealion_gone" },
    { NULL, NULL }
};

struct dialog_t dlg_grab_battery[] = {
    { "player", "I'm having this!" },
    { NULL, NULL }
};



enum dlg_state_t {
    DIALOG_STATE_READY,
    DIALOG_STATE_RUN_DIALOG,
    DIALOG_STATE_DISPLAYING_SPEECH,
    DIALOG_STATE_SENDING_EVENT,
    DIALOG_STATE_UNDEFINED,
};

struct {
    struct dialog_t* dlg;
    enum dlg_state_t current_state;
    int current_index;
} dialog;


enum dlg_state_t dialogstate( void ) {
    return dialog.current_state;
}


void startdialog( struct dialog_t* dlg ) {
    dialog.dlg = dlg;
    dialog.current_index = 0;
    dialog.current_state = DIALOG_STATE_RUN_DIALOG;
}


void rundialogline( void) {
	if( dialog.dlg[ dialog.current_index ].actor == NULL ) {
		dialog.current_state = DIALOG_STATE_READY;
        dialog.dlg = NULL;
		return;
	}

    char const* actor = dialog.dlg[ dialog.current_index ].actor;
	if( *actor != '#' ) {
		dialog.current_state = DIALOG_STATE_DISPLAYING_SPEECH;
	} else  {
		dialog.current_state = DIALOG_STATE_SENDING_EVENT;
	}
}


void execdialog( void ) {
	switch( dialog.current_state ) {
		case DIALOG_STATE_UNDEFINED: break;

		case DIALOG_STATE_READY: break;

		case DIALOG_STATE_RUN_DIALOG: {
			rundialogline();
		} break;
		
		case DIALOG_STATE_DISPLAYING_SPEECH: {
            ++dialog.current_index;
			rundialogline();
		} break;

		case DIALOG_STATE_SENDING_EVENT: {
            ++dialog.current_index;
			dialog.current_state = DIALOG_STATE_RUN_DIALOG;
		} break;
	}
}


char const* dialogactor( void ) {
    return dialog.dlg[ dialog.current_index ].actor;
}


char const* dialogline( void ) {
    return dialog.dlg[ dialog.current_index ].line;
}


void delay( int jiffys ) {
    for( int i = 0; i < jiffys; ++i ) {
        waitvbl();
    	  if( shuttingdown() || *readkeys() == KEY_ESCAPE ) break;
        drawsprites();
        swapbuffers();
    }
}


int is_near( int spr, int xpos ) {
	return ( xpos > sprite_x( spr ) - 25 ) && ( xpos < sprite_x( spr ) + 25 );
}


void set_interact( int spr, char const* str, int interact, int offset ) {
    label_text( interact, str );
	sprite_pos( interact, sprite_x( spr ) + 160 + offset, sprite_y( interact ) );
}

#define COLOR_WHITE 31
#define COLOR_GREEN 75
#define COLOR_YELLOW 185
#define COLOR_CYAN 202
#define COLOR_ORANGE 167

// PORT: at file scope rather than inside setup_speech, which Calypsi has been
// seen to miscompile (../SearchAndRescue/CLAUDE.md).
static const struct {
    char const* name;
    int x;
    int y;
    int color;
    int wrap;
} speech_settings[] = {
    { "player_sitting", 50, 85, COLOR_GREEN, 220 },
    { "player", 20, 60, COLOR_GREEN, 220 }, 
    { "player_hut", 100, 60, COLOR_GREEN, 220 }, 
    { "player_sack", 50, 20, COLOR_GREEN, 220 }, 
    { "tv_guy", 20, 20, COLOR_YELLOW, 220 }, 
    { "book_guy", 20, 20, COLOR_CYAN, 220 }, 
    { "sealion", 185, 75, COLOR_ORANGE, 120 }, 
};

void setup_speech( int speech, char const* character )
	{
    for( int i = 0; i < sizeof( speech_settings ) / sizeof( *speech_settings ) ; ++i ) {
        if( strcmp( speech_settings[ i ].name, character ) == 0 ) {
            sprite_pos( speech, speech_settings[ i ].x, speech_settings[ i ].y );
            label_color( speech, speech_settings[ i ].color );
            label_wrap( speech, speech_settings[ i ].wrap );
            return;
        }
    }
}


// PORT: ease_in_out_quad, ease_out_quad, ease_out_elastic and ease_out_bounce
// are tables of the positions they produce: see tools/convtables.py.


void title_screen( void ) {
    spr_index = 0;
    // PORT: the background and the treeline at -1050, as one picture.
    sprite( spr_index++, 0, 0, TITLE );
    int face = sprite( spr_index++, 0, 240, FACE ) ;
    int logo = sprite( spr_index++, 0, -200, LOGO ) ;
    // PORT: at 172 rather than 192. The original's credits are 6 rows above
    // the bottom of its 240; the MEGA65's screen ends at the original's 220,
    // so they keep their 6 rows from that edge instead.
    int credits2 = label( spr_index++, -160, 168, "Written for the MEGA65 by Johan Berntsson", COLOR_WHITE );
    int credits1 = label( spr_index++, -160, 180, "Based on dos-like by Mattias Gustavsson", COLOR_GREEN );

    int face_delay = 6;
    int face_move = 0;
    int logo_delay = 30;
    int logo_move = 0;
    int credits_delay = 30;
    int credits_move = 0;

	while( !shuttingdown() ) {
        waitvbl();
		if( keystate( KEY_ESCAPE ) ) break; 
		if( keystate( KEY_SPACE ) ) return;

        if( face_delay >= 0 ) {
            --face_delay;
        } else if( face_move <= 120 ) {
            int y = ease_face_y[ face_move ];  // PORT
            sprite_pos( face, sprite_x( face ), y );
            ++face_move;
        }

        if( logo_delay >= 0 ) {
            --logo_delay;
        } else if( logo_move <= 60 ) {
            // PORT: 16 rows lower, or the screen's top edge cuts it: it
            // lands at 10, the logo's top row is 4 rows into its file, and
            // the MEGA65 shows the original's rows 20..219.
            int y = ease_logo_y[ logo_move ] + 16;
            sprite_pos( logo, sprite_x( logo ), y );
            ++logo_move;
        }

        if( credits_delay >= 0 ) {
            --credits_delay;
        } else if( credits_move <= 60 ){
            int x = ease_credits_x[ credits_move ];  // PORT
            sprite_pos( credits1, x, sprite_y( credits1 ) );
            sprite_pos( credits2, x, sprite_y( credits2 ) );
            ++credits_move;
        }
        drawsprites();
        swapbuffers();
	}
    exit( 0 );

}


void game_main( void ) {  // PORT: was main; see src/main.c
    // PORT: setvideomode, setdoublebuffer, loadbitmaps, the palette, the font
    // and the music are dos_init, src/main.c's loading, tools/labels.py and
    // -- for the music -- nothing yet.
    dos_init();
    // PORT: the game starts again after the ending, so what the original
    // relied on a fresh program to have zeroed is zeroed here.
    memset( sprites, 0, sizeof( sprites ) );
    memset( &dialog, 0, sizeof( dialog ) );
    spr_index = 0;

	title_screen();
  
    int objects[ 32 ] = { 0 };
    int objects_count = 0;

    spr_index = 0;
    int o = 30;
    int background = sprite( spr_index++, -65, 0, BACKGROUND );
    int treeline = sprite( spr_index++, -65, 0, TREELINE );
    int sack = sprite( spr_index++, 240, 10 + o, SACK_SPR ); objects[ objects_count++ ] = sack;
    int hut1 = sprite( spr_index++, 540, 10 + o, HUT1 ); objects[ objects_count++ ] = hut1;
	int hut2 = sprite( spr_index++, 850, 20 + o, HUT2 ); objects[ objects_count++ ] = hut2;
	objects[ objects_count++ ] =  sprite( spr_index++, 180, -30 + o, TREE2 );
	objects[ objects_count++ ] = sprite( spr_index++, 120, -10 + o, PALMTREE );
	int chair = sprite( spr_index++, 65, 25 + o, CHAIR ); objects[ objects_count++ ] = chair;
	int sealion_battery = sprite( spr_index++, 970 + 62, -10 + o, SEALION_BATTERY ); objects[ objects_count++ ] = sealion_battery;
	int sealion = sprite( spr_index++, 970, -10 + o, SEALION ); objects[ objects_count++ ] = sealion;

	int xpos = 65;
	int santa = sprite( spr_index++, xpos, 25 + o, SANTA_CHAIR );
    
	int interact = label( spr_index++, 160, 70, "", COLOR_WHITE );
    
    int sack_back = sprite( spr_index++, -250 + o, 0, BACKGROUND );
    sprite_hide( sack_back );

    int sack_pic = sprite( spr_index++, 0, 0, SACK_FULL );
    sprite_hide( sack_pic );

	int hut_tv_pic = sprite( spr_index++, 0, 0, HUT_TV );
    sprite_hide( hut_tv_pic );

	int hut_bulb = sprite( spr_index++, 0, 0, HUT_BOOK );
    sprite_hide( hut_bulb );
        	
    int santa_inside = sprite( spr_index++, 0, 0 + o, SANTA_IDLE );
    sprite_hide( santa_inside );

    int boat = sprite( spr_index++, -320, 30 + o, BOAT );

	int speech = label( spr_index++, 50, 70, "", COLOR_GREEN );
    label_wrap( speech, 220 );


	int fadeout = 0;
  int fadeout_delay = 120;
  int speech_visible = 1;
	int rescued = 0;
	int may_stand = 0;
	int looked_at_sack = 0;
	int sitting = 1;
	int sealion_gone = 0;
	int at_sack = 0;
	int at_hut_tv = 0;
	int at_hut_bulb = 0;
	int has_dish = 0;
	int has_bulb = 0;
	int has_battery = 0;	  
  int shown_intro = 0;
  int boat_delay = 0;
  int boat_move = 0;

    // PORT: anim += 20 / 60 a frame, counted in thirds.
    int anim3 = 0;

	while( !shuttingdown() && !keystate( KEY_ESCAPE ) ) {
		for( int i = 0; i < objects_count; ++i )
            sprite_origin( objects[ i ], xpos, 0 );

		sprite_origin( interact, xpos, 0 );

		sprite_origin( santa, xpos, 0 );
		sprite_pos( santa, xpos, sprite_y( santa ) );
		sprite_pos( background, -xpos, sprite_y( background ) );
		sprite_pos( treeline, -xpos, sprite_y( treeline ) );			

        if( !shown_intro ) {
            delay( 60 );
            startdialog( dlg_lets_go_home );
            shown_intro = 1;
        }

        anim3 = ( anim3 + 1 ) % ( 3 * 12 );  // PORT: every animation is 12 cels or 1
        sprite_cel( santa, anim3 / 3 );

        waitvbl();
    
		if( fadeout ) {
            if( --fadeout_delay <= 0 ) {
			    int all_black = 1;
			    for( int i = 0; i < 256; ++i ) {
                    int r, g, b;
                    getpal( i, &r, &g, &b );
				    if( r > 0 || g > 0 || b > 0 ) all_black = 0;
                    if( b > 0 ) --b;
                    else if( g > 0 ) --g;
                    else if( r > 0 ) --r;
				    setpal( i, r, g, b );
			    }
			    if( all_black ) break;;
            }
		}
		
		if( keystate( KEY_ESCAPE ) ) break;


        if( dialogstate() != DIALOG_STATE_READY ) {
			if( dialogstate() == DIALOG_STATE_SENDING_EVENT ) {
   				char const* event = dialogactor();
				if( strcmp( event, "#SetFlag" ) == 0 ) {
					char const* flag = dialogline();
					if( strcmp( flag, "may_stand" ) == 0 ) {
						may_stand = 1;
					}
					if( strcmp( flag, "looked_at_sack" ) == 0 ) {
						looked_at_sack = 1;
					}
					if( strcmp( flag, "sealion_running" ) == 0 ) {
                        sprite_pos( sealion, 970 + 62, sprite_y( sealion ) );
                        sprite_bitmap( sealion, SEALION_WALK );
						delay( 30 );
                        for( int i = 0; i < 90; ++i ) {
                            waitvbl();
                            // PORT: penganim += 25 / 60 a frame
                            sprite_cel( sealion, ( i + 1 ) * 25 / 60 );
                            sprite_pos( sealion, sprite_x( sealion ) + 3, sprite_y( sealion ) );
                            drawsprites();
                            swapbuffers();
                        }
                        sprite_hide( sealion );
					}
					if( strcmp( flag, "sealion_gone" ) == 0 ) {
						sealion_gone = 1;
						execdialog();
					}
					if( strcmp( flag, "rescued" ) == 0 ) {
						rescued = 1;
					}
				}
				if( strcmp( event, "#ClearFlag" ) == 0 ) {
					char const* flag = dialogline();
					if( strcmp( flag, "at_hut_tv" ) == 0 ) {
						sprite_hide( hut_tv_pic );
						sprite_hide( santa_inside );
						sprite_bitmap( hut_tv_pic, HUT_TV_NODISH );
						has_dish = 1;
						at_hut_tv = 0;
					}
					if( strcmp( flag, "at_hut_bulb" ) == 0 ) {
						sprite_hide( hut_bulb );
						sprite_hide( santa_inside );
						sprite_bitmap( hut_bulb, HUT_BOOK_NOBULB );
						has_bulb = 1;
						at_hut_bulb = 0;
					}
				}
 				execdialog();
			} else if( dialogstate() == DIALOG_STATE_DISPLAYING_SPEECH ) {
                sprite_bitmap( santa, sitting ? SANTA_CHAIR : SANTA_IDLE );
				setup_speech( speech, dialogactor() );
				label_text( speech, dialogline() );
				if( speech_visible && *readkeys() == KEY_SPACE )  {					
					speech_visible = 0;
                    label_text( speech, "" ); 
                    delay( 30 );
					execdialog(); 
                    speech_visible = 1;
				}
			} else {
                execdialog();
			}
            drawsprites();
            swapbuffers();
            continue;
		} else {
		    label_text( speech, "" ); 
		}
		   
        label_text( interact, "" ); 
		if( !sitting && !at_sack && !at_hut_tv && !at_hut_bulb && !rescued ) {
			if( is_near( sack, xpos ) ) set_interact( sack, "Sack", interact, 0 );
			if( is_near( hut1, xpos ) ) set_interact( hut1, "Hut", interact, 0 );
			if( is_near( hut2, xpos ) ) set_interact( hut2, "Hut", interact, 0 );
			if( !sealion_gone && is_near( sealion, xpos ) ) set_interact( sealion, "Sea lion", interact, 60 );
			if( !has_battery && sealion_gone && is_near( sealion_battery, xpos ) ) set_interact( sealion_battery, "Battery", interact, 0 );
				
			if( keystate( KEY_LEFT ) )  {
				xpos -= 2;
				if( xpos < 10 ) xpos = 10;
                sprite( santa, sprite_x( santa ), sprite_y( santa ), SANTA_LEFT );
			} else if( keystate( KEY_RIGHT ) ) {
				xpos += 2;
				if( xpos > 1120 ) xpos = 1120;
                sprite( santa, sprite_x( santa ), sprite_y( santa ), SANTA_RIGHT );
			} else {
                sprite( santa, sprite_x( santa ), sprite_y( santa ), SANTA_IDLE );
			}

			if( *readkeys() == KEY_SPACE ) {
				if( is_near( sack, xpos ) ) {
					sprite_show( sack_pic );
					if( has_dish && has_bulb && has_battery ) {
                        sprite_bitmap( sack_pic, SACK_FULL );
						startdialog( dlg_sack_all );
					} else if( has_dish && has_bulb ) {
                        sprite_bitmap( sack_pic, SACK_BULB_DISH );
						startdialog( dlg_sack_bulb_dish );
					} else if( has_battery && has_bulb ) {
                        sprite_bitmap( sack_pic, SACK_BULB_BATTERY );
						startdialog( dlg_sack_bulb_battery );
					} else if( has_dish && has_battery ) {
                        sprite_bitmap( sack_pic, SACK_BATTERY_DISH );
						startdialog( dlg_sack_battery_dish );
					} else if( has_dish ) {
                        sprite_bitmap( sack_pic, SACK_DISH );
						startdialog( dlg_sack_dish );
					} else if( has_battery ) {
                        sprite_bitmap( sack_pic, SACK_BATTERY );
						startdialog( dlg_sack_battery );
					} else if( has_bulb ) {
                        sprite_bitmap( sack_pic, SACK_BULB );
						startdialog( dlg_sack_bulb );
					} else {
                        sprite_bitmap( sack_pic, SACK_NONE );
						if( !looked_at_sack )
							startdialog( dlg_sack_first );
						else
							startdialog( dlg_sack_none );
					}

					sprite_show( sack_back );
					at_sack = 1;
				}
				
                if( is_near( hut1, xpos ) ) {
					sprite_show( hut_tv_pic );
					sprite_show( santa_inside );
                    sprite_pos( santa_inside, 100, 60 );
					if( !has_dish )
						startdialog( dlg_hut_tv );
					else
						startdialog( dlg_hut_tv_sleep );
					at_hut_tv = 1;
				}

				if( is_near( hut2, xpos ) ) {
					sprite_show( hut_bulb );
					sprite_show( santa_inside );
                    sprite_pos( santa_inside, 100, 60 );
					if( !has_bulb )
						startdialog( dlg_hut_book );
					else
						startdialog( dlg_hut_book_sleep );
					at_hut_bulb = 1;
				}

				if( !sealion_gone && is_near( sealion, xpos ) ) {
                    sprite_bitmap( sealion, SEALION );
					startdialog( dlg_sealion );
					label_text( interact, "" );
                    sprite_bitmap( santa, SANTA_IDLE );
				}

				if( !has_battery && sealion_gone && is_near( sealion_battery, xpos ) ) {
					label_text( interact, "" );
					sprite_hide( sealion );
					startdialog( dlg_grab_battery );
					sprite_hide( sealion_battery );
					has_battery = 1;
				}

                if( is_near( chair, xpos ) ) {
					sitting = 1;
					xpos = 65;
					sprite_bitmap( santa, SANTA_CHAIR );
                    sprite_pos( santa, 0, 25+o );
		            sprite_origin( santa, xpos, 0 );
		            sprite_pos( santa, xpos, sprite_y( santa ) );
				}
			}

		} else if( sitting && may_stand ) {
			if( *readkeys() == KEY_SPACE ) {
				sitting = 0;
                sprite( santa, 0, 30+o, SANTA_IDLE ); 
			}
		} else if( at_sack ) {
			if( *readkeys() == KEY_SPACE ) {
				sprite_hide( sack_pic );
				sprite_hide( sack_back );
				at_sack = 0;

                if( rescued ) {
                    sprite_bitmap( santa, SANTA_JUMP );
					xpos = 20;
		            sprite_origin( santa, xpos, 0 );
		            sprite_pos( santa, xpos, sprite_y( santa ) );
                    boat_delay = 90;
                    boat_move = 0;
					fadeout = 1; 
				}
			}
		} else if( at_hut_tv ) {
			if( *readkeys() == KEY_SPACE ) {
			    sprite_hide( hut_tv_pic );
			    sprite_hide( santa_inside );
				sprite_bitmap( hut_tv_pic, HUT_TV_NODISH );
				has_dish = 1;
				at_hut_tv = 0;
			}
		} else if( at_hut_bulb ) {
			if( *readkeys() == KEY_SPACE ) {
				sprite_hide( hut_bulb );
				sprite_hide( santa_inside );
				sprite_bitmap( hut_bulb, HUT_BOOK_NOBULB );
				has_bulb = 1;
				at_hut_bulb = 0;
            }
        }

        execdialog();

        if( rescued && fadeout ) {
            if( boat_delay >= 0 ) {
                --boat_delay;
            } else {
                int x = ease_boat_x[ boat_move ];  // PORT
                sprite_pos( boat, x, sprite_y( boat ) );
                if( boat_move < EASE_BOAT_X_LEN - 1 ) ++boat_move;
            }
        }

		sprite_origin( santa, xpos, 0 );
		sprite_pos( santa, xpos, sprite_y( santa ) );
        drawsprites();
        swapbuffers();
	}

}

/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2021 Mattias Gustavsson

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.

------------------------------------------------------------------------------

ALTERNATIVE B - Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------
*/
