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
#define SACK_SPR   INVALID
#define HOUSE_BEAR INVALID
#define HOUSE_FOX  INVALID
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

struct dialog_t dlg_sack_cylinder[] = {
    { "player_sack", "It is missing the ANTENNA and the BATTERY." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_green[] = {
    { "player_sack", "It is missing the BULB and the BATTERY." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_blue[] = {
    { "player_sack", "It is missing the ANTENNA and the BULB." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_cylinder_green[] = {
    { "player_sack", "It is missing the BATTERY." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_blue_green[] = {
    { "player_sack", "It is missing the BULB." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_cylinder_blue[] = {
    { "player_sack", "It is missing the ANTENNA." },
    { NULL, NULL }
};

struct dialog_t dlg_sack_all[] = {
    { "player_sack", "The radio is complete! Now I can go home." },
    { "player_sack", "MAYDAY! MAYDAY! I AM STRANDED ON AN ISLAND. SEND HELP!" },
    { "#SetFlag", "rescued" },
    { NULL, NULL }
};

struct dialog_t dlg_bear[] = {
    { "bear", "Shh! I'm watching." },
    { "player_hut", "Watching what?" },
    { "bear", "The Yule Log channel." },
    { "bear", "Same log since April '92." },
    { "player_hut", "Have you tried another channel?" },
    { "bear", "With *what*? My remote is gone." },
    { "player_hut", "Is that my GREEN present by your chair?" },
    { "bear", "Fell down the chimney. Best thing on all day." },
    { "bear", "It stays until I can change the channel." },
    { "player_hut", "Are those cookies?" },
    { "bear", "Take them. I can't eat with the TV like this." },
    { "bear", "Doctor's orders." },
    { "player_hut", "These are for... someone else. Strictly." },
    { "#SetFlag", "has_cookies" },
    { NULL, NULL }
};

struct dialog_t dlg_bear_again[] = {
    { "bear", "Still the log. It crackled once. Big moment." },
    { NULL, NULL }
};

struct dialog_t dlg_bear_remote[] = {
    { "player_hut", "One remote, slightly foxed." },
    { "#SetFlag", "gave_remote" },
    { "bear", "My remote! Channel two... channel three..." },
    { "bear", "It's the Yule Log channel. From a different angle." },
    { "bear", "Bliss. Take your present, I'm busy." },
    { "#SetFlag", "has_green" },
    { NULL, NULL }
};

struct dialog_t dlg_bear_after[] = {
    { "bear", "Shh. The log is at the good part." },
    { NULL, NULL }
};

struct dialog_t dlg_fox[] = {
    { "fox", "Hello, stranger. Nothing to see here." },
    { "player_hut", "That's the bear's remote." },
    { "fox", "I *borrowed* it. It doesn't work." },
    { "fox", "I point it at the window. Same snow every day." },
    { "player_hut", "That's a window." },
    { "fox", "Very low resolution." },
    { "player_hut", "And that's my GOLD present." },
    { "fox", "Finders keepers. Can't think when I'm this hungry." },
    { "player_hut", "Let me check my list. Twice." },
    { "player_hut", "Fox. Line 10: NAUGHTY. Line 20: GOTO 10." },
    { "#SetFlag", "met_fox" },
    { NULL, NULL }
};

struct dialog_t dlg_fox_again[] = {
    { "fox", "My stomach is rumbling. Something sweet would help." },
    { NULL, NULL }
};

// Leaving after this takes the remote and the present.
struct dialog_t dlg_fox_cookies[] = {
    { "player_hut", "Cookies, for a very naughty fox." },
    { "fox", "Cookies! Take whatever you like. I won't notice." },
    { "#SetFlag", "fox_eating" },
    { "fox", "*munch* I didn't see a thing." },
    { NULL, NULL }
};

struct dialog_t dlg_fox_after[] = {
    { "fox", "*munch* Mmf. Go away. *munch*" },
    { NULL, NULL }
};

struct dialog_t dlg_sealion[] = {
    { "sealion", "Arf. Go away. I'm sulking." },
    { "player", "Is that my BLUE present behind you?" },
    { "sealion", "It fell out of the sky. I'm keeping it." },
    { "sealion", "I lost my ball. That box is all I've got left." },
    { "player", "You can't play with a box." },
    { "sealion", "I tried balancing it. It's square." },
    { "player", "Can I have it back?" },
    { "sealion", "Not until I've got something else to play with." },
    { "#SetFlag", "met_sealion" },
    { NULL, NULL }
};

struct dialog_t dlg_sealion_again[] = {
    { "sealion", "There was a penguin here once. Licked batteries." },
    { "sealion", "Weird guy." },
    { NULL, NULL }
};

struct dialog_t dlg_sealion_ball[] = {
    { "player", "Would you trade for a ball?" },
    { "sealion", "MY BALL! Where was it?" },
    { "player", "In a tree." },
    { "sealion", "Classic ball." },
    { "sealion", "Off to practise. The present's yours. Arf!" },
    { "#SetFlag", "sealion_running" },
    { "player", "Ho ho. A nose for business." },
    { "#SetFlag", "sealion_gone" },
    { NULL, NULL }
};

struct dialog_t dlg_grab_blue[] = {
    { "player", "Slightly fishy. Nobody will notice." },
    { NULL, NULL }
};

struct dialog_t dlg_tree[] = {
    { "player_tree", "A ball, up in a tree. In winter." },
    { "player_tree", "I can't reach it." },
    { "player_tree", "If only this game had a jump button." },
    { "player_tree", "Oh well. HO HO HO!" },
    { "#SetFlag", "ball_fell" },
    { "player_tree", "Works every time." },
    { NULL, NULL }
};

struct dialog_t dlg_grab_ball[] = {
    { "player", "A beach ball. Someone here has big summer plans." },
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
    { "player_tree", 20, 20, COLOR_GREEN, 220 },  // above the ball
    { "bear", 20, 20, COLOR_YELLOW, 220 }, 
    { "fox", 20, 20, COLOR_CYAN, 220 }, 
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
    int house_bear = sprite( spr_index++, 540, 10 + o, HOUSE_BEAR ); objects[ objects_count++ ] = house_bear;
	int house_fox = sprite( spr_index++, 850, 20 + o, HOUSE_FOX ); objects[ objects_count++ ] = house_fox;
	// The ball's tree is part of WORLD too, and this sprite of it is shown
	// only while it shakes.
	int tree = sprite( spr_index++, 180, -30 + o, TREE2 ); objects[ objects_count++ ] = tree;
	sprite_hide( tree );
	objects[ objects_count++ ] = sprite( spr_index++, 120, -10 + o, PALMTREE );
	int chair = sprite( spr_index++, 65, 25 + o, CHAIR ); objects[ objects_count++ ] = chair;
	int gift_blue = sprite( spr_index++, 970 + 62, -10 + o, GIFT_BLUE ); objects[ objects_count++ ] = gift_blue;
	int sealion = sprite( spr_index++, 970, -10 + o, SEALION ); objects[ objects_count++ ] = sealion;

	int xpos = 65;
	int santa = sprite( spr_index++, xpos, 25 + o, SANTA_CHAIR );

	// After Santa, so that it lands in front of his feet.
	int ball = sprite( spr_index++, 180, -10 + o, BALL_IN_TREE ); objects[ objects_count++ ] = ball;

	int interact = label( spr_index++, 160, 70, "", COLOR_WHITE );
    
    int sack_back = sprite( spr_index++, -250 + o, 0, BACKGROUND );
    sprite_hide( sack_back );

    int sack_pic = sprite( spr_index++, 0, 0, SACK_FULL );
    sprite_hide( sack_pic );

	int hut_bear_pic = sprite( spr_index++, 0, 0, HUT_BEAR );
    sprite_hide( hut_bear_pic );
	int gift_green = sprite( spr_index++, 0, -10 + o, GIFT_GREEN );
    sprite_hide( gift_green );

	int hut_fox_pic = sprite( spr_index++, 0, 0, HUT_FOX );
    sprite_hide( hut_fox_pic );
	int gift_cylinder = sprite( spr_index++, 0, -10 + o, GIFT_CYLINDER );
    sprite_hide( gift_cylinder );
        	
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
	int at_hut_bear = 0;
	int at_hut_fox = 0;
	int has_green = 0;
	int has_cylinder = 0;
	int has_blue = 0;
	int ball_fell = 0;
	int met_sealion = 0;
	int has_cookies = 0;
	int met_fox = 0;
	int fox_eating = 0;
	int has_remote = 0;
	int has_ball = 0;
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
					if( strcmp( flag, "has_cookies" ) == 0 ) {
						has_cookies = 1;
					}
					if( strcmp( flag, "gave_remote" ) == 0 ) {
						sprite_bitmap( hut_bear_pic, HUT_BEAR_NOGREEN );
					}
					if( strcmp( flag, "has_green" ) == 0 ) {
						sprite_hide( gift_green );
						has_green = 1;
					}
					if( strcmp( flag, "met_fox" ) == 0 ) {
						met_fox = 1;
					}
					if( strcmp( flag, "fox_eating" ) == 0 ) {
						sprite_bitmap( hut_fox_pic, HUT_FOX_NOCYLINDER );
						fox_eating = 1;
					}
					if( strcmp( flag, "met_sealion" ) == 0 ) {
						met_sealion = 1;
					}
					if( strcmp( flag, "sealion_running" ) == 0 ) {
						// He leaves with the ball: SEALION_WALK has it on his nose.
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
					if( strcmp( flag, "ball_fell" ) == 0 ) {
						// Santa's cheer shakes the tree, two pixels each way,
						// and then the ball drops out of it and bounces.
						sprite_bitmap( santa, SANTA_JUMP );
						sprite_show( tree );
						for( int i = 0; i < 36; ++i ) {
							waitvbl();
							sprite_cel( santa, i / 3 );
							sprite_pos( tree, 180 + ( ( i & 4 ) ? 2 : -2 ), sprite_y( tree ) );
							drawsprites();
							swapbuffers();
						}
						sprite_pos( tree, 180, sprite_y( tree ) );
						sprite_hide( tree );
						sprite_bitmap( santa, SANTA_IDLE );
						for( int i = 0; i < EASE_BALL_Y_LEN; ++i ) {
							waitvbl();
							sprite_pos( ball, 180, -10 + o + ease_ball_y[ i ] );
							drawsprites();
							swapbuffers();
						}
						sprite_bitmap( ball, BALL_ON_SNOW );
						sprite_pos( ball, 180, -10 + o );
						ball_fell = 1;
					}
					if( strcmp( flag, "sealion_gone" ) == 0 ) {
						sealion_gone = 1;
						execdialog();
					}
					if( strcmp( flag, "rescued" ) == 0 ) {
						rescued = 1;
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
		if( !sitting && !at_sack && !at_hut_bear && !at_hut_fox && !rescued ) {
			if( is_near( sack, xpos ) ) set_interact( sack, "Sack (lighter than it should be)", interact, 0 );
			if( !has_ball && is_near( tree, xpos ) ) set_interact( tree, "Ball", interact, 0 );
			if( has_ball && is_near( tree, xpos ) ) set_interact( tree, "Tree (undecorated)", interact, 0 );
			if( is_near( house_bear, xpos ) ) set_interact( house_bear, "Bear's den", interact, 0 );
			if( is_near( house_fox, xpos ) ) set_interact( house_fox, "Fox's den", interact, 0 );
			if( !sealion_gone && is_near( sealion, xpos ) ) set_interact( sealion, "Sea lion (sulking)", interact, 60 );
			if( !has_blue && sealion_gone && is_near( gift_blue, xpos ) ) set_interact( gift_blue, "Present", interact, 0 );
				
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
					if( has_green && has_cylinder && has_blue ) {
                        sprite_bitmap( sack_pic, SACK_FULL );
						startdialog( dlg_sack_all );
					} else if( has_green && has_cylinder ) {
                        sprite_bitmap( sack_pic, SACK_CYLINDER_GREEN );
						startdialog( dlg_sack_cylinder_green );
					} else if( has_blue && has_cylinder ) {
                        sprite_bitmap( sack_pic, SACK_CYLINDER_BLUE );
						startdialog( dlg_sack_cylinder_blue );
					} else if( has_green && has_blue ) {
                        sprite_bitmap( sack_pic, SACK_BLUE_GREEN );
						startdialog( dlg_sack_blue_green );
					} else if( has_green ) {
                        sprite_bitmap( sack_pic, SACK_GREEN );
						startdialog( dlg_sack_green );
					} else if( has_blue ) {
                        sprite_bitmap( sack_pic, SACK_BLUE );
						startdialog( dlg_sack_blue );
					} else if( has_cylinder ) {
                        sprite_bitmap( sack_pic, SACK_CYLINDER );
						startdialog( dlg_sack_cylinder );
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

				if( !ball_fell && is_near( tree, xpos ) ) {
					label_text( interact, "" );
					startdialog( dlg_tree );
				} else if( !has_ball && is_near( tree, xpos ) ) {
					label_text( interact, "" );
					startdialog( dlg_grab_ball );
					sprite_hide( ball );
					has_ball = 1;
				}
				
                if( is_near( house_bear, xpos ) ) {
					sprite_show( hut_bear_pic );
					if( !has_green ) sprite_show( gift_green );
					sprite_show( santa_inside );
                    sprite_pos( santa_inside, 100, 60 );
					if( !has_cookies )
						startdialog( dlg_bear );
					else if( has_green )
						startdialog( dlg_bear_after );
					else if( has_remote )
						startdialog( dlg_bear_remote );
					else
						startdialog( dlg_bear_again );
					at_hut_bear = 1;
				}

				if( is_near( house_fox, xpos ) ) {
					sprite_show( hut_fox_pic );
					if( !has_cylinder ) sprite_show( gift_cylinder );
					sprite_show( santa_inside );
                    sprite_pos( santa_inside, 100, 60 );
					if( !met_fox )
						startdialog( dlg_fox );
					else if( fox_eating )
						startdialog( dlg_fox_after );
					else if( has_cookies )
						startdialog( dlg_fox_cookies );
					else
						startdialog( dlg_fox_again );
					at_hut_fox = 1;
				}

				if( !sealion_gone && is_near( sealion, xpos ) ) {
                    sprite_bitmap( sealion, SEALION );
					if( !met_sealion )
						startdialog( dlg_sealion );
					else if( has_ball )
						startdialog( dlg_sealion_ball );
					else
						startdialog( dlg_sealion_again );
					label_text( interact, "" );
                    sprite_bitmap( santa, SANTA_IDLE );
				}

				if( !has_blue && sealion_gone && is_near( gift_blue, xpos ) ) {
					label_text( interact, "" );
					sprite_hide( sealion );
					startdialog( dlg_grab_blue );
					sprite_hide( gift_blue );
					has_blue = 1;
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
		} else if( at_hut_bear ) {
			if( *readkeys() == KEY_SPACE ) {
			    sprite_hide( hut_bear_pic );
			    sprite_hide( gift_green );
			    sprite_hide( santa_inside );
				at_hut_bear = 0;
			}
		} else if( at_hut_fox ) {
			if( *readkeys() == KEY_SPACE ) {
				sprite_hide( hut_fox_pic );
				sprite_hide( gift_cylinder );
				sprite_hide( santa_inside );
				at_hut_fox = 0;
				// The fox is too busy with the cookies to notice.
				if( fox_eating && !has_cylinder ) {
					has_remote = 1;
					has_cylinder = 1;
				}
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
