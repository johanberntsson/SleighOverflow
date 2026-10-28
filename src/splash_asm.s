; Where the Kernal's screen editor will print next, from its PLOT call with
; the carry set: X the row, Y the column. src/splash.c draws its cursor there.

            .section code,text
            .public kernal_row, kernal_column

kernal_row:
            sec
            jsr     0xfff0
            txa
            rts

kernal_column:
            sec
            jsr     0xfff0
            tya
            rts
