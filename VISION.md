# Modifications and additional puzzles

You see a ball high up in the trees (and "ball" text appears).
Try to click and it says that you can't reach. Jump and it falls
down, and then you can pick it up.

You say you want the present, the sea lion
says that it's keeping it to cheer it up since he lost his ball. Give
him the ball and he goes away, balancing the ball, and you get the present.

One of the huts now has a icebear watching TV. He says he's bored watching
the same channel. You ask why and he says that his remote is gone. He's got
one present and says its the only thing keeping him happy while the TV is on
the fritz. There's also a table with some food. You ask if he minds, but he
says he's lost his appetite. You can pick it up

The next hut as someone (haven't decided) and a remote. You ask and that
one says he "borrowed" it but it didn't work with his TV set. You ask for it
back he refuses but gives a hint that he's hungry. If you have food you give it
and he starts eating. You can then take both the remote and the present. You
can then go back to the icebear and give the remote and get the last present
and go home. Perhaps a fox? 

# Background

I'm making a point and click graphical adventure (small) as a christmas demo. It's just a
long scrolling picture with snow on the ground and a forest in the background. As you walk by 
there are two houses, and finally a sea lion by the beach. The story is that you (santa - a 
walking animated large sprite) have dropped three christmas presents from the sleigh as you 
flew by, and you need to interact with the sea lion and the people in the two houses to get 
them back for the children who are waiting (solving some mini problems in the process). 
This is like a christmas demo/suprise for the MEGA65 retro computing community.

The game is called "ERROR 65: SLEIGH OVERFLOW", or Sleigh Overflow for short.

The splash screen could be something like:

    THE MEGA65 PERSONAL COMPUTER SYSTEM
    BASIC 65 V920413

    READY.
    DLOAD "SANTA"

    SEARCHING FOR SANTA
    LOADING
    READY.
    RUN

    ERROR 65 IN 2026: SLEIGH OVERFLOW

Then we'd have a picture of Santa dropping a sack, and start the game with Santa by the sack,
discovering that three presents are missing. And finally when you got all the presents, you
return to the sack and get a final picture of Santa on his way.




# Dialogue draft

A first draft, to cut and rewrite. Lines are kept under about 60
characters so they fit the speech label; a longer thought is split over
two lines. `[flag]` is where a `#SetFlag` would go. Each scene is entered
with SPACE, as Stranded's huts and radio are, and what is said depends on
the flags. Carried items are only flags, never drawn in Santa's hands.

Who has what:

| Present | Who has it | What they want |
|---|---|---|
| the GREEN one | the bear | his remote back (the fox has it) |
| the GOLD one (the cylinder) | the fox | something to eat (the bear's cookies) |
| the BLUE one | the sea lion | his ball (up in a tree) |

## The start, by the sack

After the splash and the picture of Santa dropping the sack:

    SANTA: Ho ho... ho?
    SANTA: That landing was rougher than usual.
    SANTA: Too many presents, not enough sleigh.
    SANTA: Classic overflow.
    SANTA: And now the sack feels light. Too light.

## The sack

First look:

    SANTA: ?3 PRESENTS MISSING ERROR
    SANTA: The GREEN one, the GOLD one and the BLUE one!
    SANTA: They must have fallen out somewhere around here.
    [looked_at_sack]

Later, by how many are still missing:

    SANTA: ?2 PRESENTS MISSING ERROR

    SANTA: ?1 PRESENT MISSING ERROR
    SANTA: So close. Don't tell the reindeer.

All three back:

    SANTA: READY.
    SANTA: All present and correct. Ho ho!
    [delivered]

## The tree

Near the tree the label is "Ball" while the ball is up there, and
"Tree (undecorated)" after. First SPACE:

    SANTA: A ball, up in a tree. In winter.
    SANTA: I can't reach it.
    SANTA: If only this game had a jump button.
    SANTA: Oh well. HO HO HO!
    [ball_fell]   (Santa's cheer, the tree shakes, the ball drops)
    SANTA: Works every time.

Picking it up:

    SANTA: A beach ball. Someone here has big summer plans.
    [has_ball]

## The bear's house (GREEN)

First visit. The cookies are taken here, so the table has no
interaction of its own:

    BEAR: Shh! I'm watching.
    SANTA: Watching what?
    BEAR: The Yule Log channel.
    BEAR: Same log since April '92.
    SANTA: Have you tried another channel?
    BEAR: With *what*? My remote is gone.
    SANTA: Is that my GREEN present by your chair?
    BEAR: Fell down the chimney. Best thing on all day.
    BEAR: It stays until I can change the channel.
    SANTA: Are those cookies?
    BEAR: Take them. I can't eat with the TV like this.
    BEAR: Doctor's orders.
    SANTA: These are for... someone else. Strictly.
    [has_cookies]

Again, without the remote:

    BEAR: Still the log. It crackled once. Big moment.

With the remote:

    SANTA: One remote, slightly foxed.
    BEAR: My remote! Channel two... channel three...
    BEAR: It's the Yule Log channel. From a different angle.
    BEAR: Bliss. Take your present, I'm busy.
    [has_green]

After:

    BEAR: Shh. The log is at the good part.

## The fox's house (GOLD)

First visit, or any visit without the cookies:

    FOX: Hello, stranger. Nothing to see here.
    SANTA: That's the bear's remote.
    FOX: I *borrowed* it. It doesn't work.
    FOX: I point it at the window. Same snow every day.
    SANTA: That's a window.
    FOX: Very low resolution.
    SANTA: And that's my GOLD present.
    FOX: Finders keepers. Can't think when I'm this hungry.
    SANTA: Let me check my list. Twice.
    SANTA: Fox. Line 10: NAUGHTY. Line 20: GOTO 10.
    [met_fox]

Again, without the cookies:

    FOX: My stomach is rumbling. Something sweet would help.

With the cookies:

    SANTA: Cookies, for a very naughty fox.
    FOX: Cookies! Take whatever you like. I won't notice.
    FOX: *munch* I didn't see a thing.
    [fox_eating]   (leaving takes the remote and the present)
    [has_remote]
    [has_cylinder]

After:

    FOX: *munch* Mmf. Go away. *munch*

## The sea lion on the beach (BLUE)

First visit, without the ball. The present is on the snow behind him:

    SEA LION: Arf. Go away. I'm sulking.
    SANTA: Is that my BLUE present behind you?
    SEA LION: It fell out of the sky. I'm keeping it.
    SEA LION: I lost my ball. That box is all I've got left.
    SANTA: You can't play with a box.
    SEA LION: I tried balancing it. It's square.
    SANTA: Can I have it back?
    SEA LION: Not until I've got something else to play with.
    [met_sealion]

Again, without the ball:

    SEA LION: There was a penguin here once. Licked batteries.
    SEA LION: Weird guy.

With the ball:

    SANTA: Would you trade for a ball?
    SEA LION: MY BALL! Where was it?
    SANTA: In a tree.
    SEA LION: Classic ball.
    SEA LION: Off to practise. The present's yours. Arf!
    [sealion_running]
    SANTA: Ho ho. A nose for business.
    [sealion_gone]

Picking up the present he leaves behind:

    SANTA: Slightly fishy. Nobody will notice.
    [has_blue]

## The end

At the sack with all three (above), then the picture of Santa on his way,
and the fade back to the blue BASIC screen of the splash:

    LIST

    10 PRINT "MERRY CHRISTMAS"
    20 GOTO 10

    READY.
    _

with the cursor blinking. The splash's font and colours make this nearly
free. The credits could be more `PRINT` lines, including a thank-you to
Mattias Gustavsson for Stranded.

# Extras, all cheap

## Hover labels

Offline bitmaps, so a joke costs nothing at run time:

| Thing | Label |
|---|---|
| the sack | Sack (lighter than it should be) |
| the tree, ball gone | Tree (undecorated) |
| the bear's house | Bear's den (cable TV) |
| the fox's house | Fox's den (no TV) |
| the sea lion | Sea lion (busy) |
| a present on the ground | BLUE present |

## Idle Santa

If Santa stands still for a while, one of these, in turn:

    SANTA: Brrr. Someone POKE 53280 to something warmer.
    SANTA: Should have pressed RUN/STOP-RESTORE on that landing.
    SANTA: Rudolph will say I should have used a fastloader.
    SANTA: Last year I ran out of memory over Finland.
    SANTA: Had to swap to attic RAM.

## The presents' tags

Read out when one is picked up, after the line above:

- GREEN: "To: a very patient kid in Gothenburg. Nice."
- GOLD: "To: someone who never borrows remotes. Nice."
- BLUE: "To: the MEGA65 community. Nice. Mostly."

## Commodore in-jokes, all in one place

- The title is a BASIC error: `ERROR 65 IN 2026` reads like `ERROR IN
  LINE 2026`.
- The sack counts down in errors, `?3 PRESENTS MISSING ERROR`, and ends on
  `READY.`
- The Yule Log channel has been on since April '92, the date of the
  splash's `BASIC 65 V920413`, the C65's last ROM.
- The naughty list is a BASIC program: `10 NAUGHTY`, `20 GOTO 10`.
- The ending `LIST`s the classic two-liner.
- POKE 53280 (the border), RUN/STOP-RESTORE, fastloaders and attic RAM in
  the idle lines.
- The sea lion remembers Stranded's penguin, a nod to the game this one is
  built on.

Other ideas:

 Dialogue draft
  - A table of who has which present (bear: GREEN, fox: GOLD, sea lion: BLUE) and what each one wants in return.
  - For each scene, the lines for each state: the first visit, coming back without the item they want, handing
    it over, and visiting afterwards. Scenes are the start, the sack, the tree, the two houses, the sea lion and
    the ending.
  - Where a #SetFlag would go, marked with names like [has_cookies] and [fox_eating]. They map straight onto
    Stranded's flags when this moves into game.c.
  - Lines are kept under about 60 characters so they fit the speech label.
  
  Extras
  - Funny hover labels for each thing you can use.
  - Lines Santa says if he stands still for a while.
  - The text on each present's tag.
  - All the Commodore in-jokes listed in one place.
  
  Some choices I made that you may want to change:
 - The bear gives you the cookies during your first talk with him, so the table needs no interaction of its
    own. That's less code and no extra label.
  - The jump is Santa's "HO HO HO!" at the tree, reusing his cheer animation, so the game needs no jump key.
  - The bear's TV channel has been on since "April '92", matching the V920413 in your splash. If you change the
    splash's version, that line needs to change too.
  - The ending screen LISTs 10 PRINT "MERRY CHRISTMAS" / 20 GOTO 10. It reuses the splash's font and colours, so
    it's nearly free, and the credits could be more PRINT lines. That includes a thank-you to Mattias
    Gustavsson for Stranded.
  
  The sea lion's line about a penguin who "licked batteries" refers to the penguin in Stranded. Cut it if you'd
  rather not mention the other game.


