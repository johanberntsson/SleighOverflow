; Back to BASIC at the end: from ../SearchAndRescue's bank.s.
;
; **It is a return, not a reset.** A reset reruns the disk's AUTOBOOT.C65,
; which is this game, so the way back is through the SYS that started it:
; the MEGA65 startup never moves the stack, and keeps the stack pointer main
; was called with in _InitialStack, for exit(). What stands in the way is
; the memory the game took from BASIC -- zero page, the stack, the screen and
; the ROM's buffers in $0002-$1FFF, and bank 1, where BASIC keeps its
; variables and the backdrop cache now is. __low_level_init copies both to
; attic RAM before anything has written over them, and basic_return copies
; them back, with the game's last joke written into them: see src/basic.c.
;
; BASIC 65's addresses were found in xemu with test programs (TODO.md,
; 28 Oct): its text pointer is at $3B, the end of its program at $82.
TXTPTR:     .equ    0x3b            ; where the interpreter reads on after SYS
TEXT_TOP:   .equ    0x82            ; the end of the program, past its 00 00
KEY_COUNT:  .equ    0xd0            ; the keyboard queue: how many
KEY_QUEUE:  .equ    0x2b0           ; and what
TEXT:       .equ    0x2001
TEXT_END:   .equ    TEXT + program_end - program

            .rtmodel version, "1"
            .rtmodel core, "*"

            .extern _InitialStack

            .section code, text
            .public __low_level_init, basic_return

; Called by the startup before anything else, with only _Vsp's two bytes of
; zero page written.
__low_level_init:
            lda     #.byte1 save_low
            ldx     #.byte0 save_low
            jsr     dma_run
            lda     #.byte1 save_bank1
            ldx     #.byte0 save_bank1
            jmp     dma_run

; Interrupts off and the display, SIDs and CIA2 already BASIC's (src/basic.c).
; Never returns to its caller. Zero page and the stack are BASIC's again after
; the second job, so nothing here may use either until the RTS.
basic_return:
            sei
            lda     #.byte1 load_bank1
            ldx     #.byte0 load_bank1
            jsr     dma_run
            ; Over the SYS line and the startup, both long done with.
            lda     #.byte1 load_text
            ldx     #.byte0 load_text
            jsr     dma_run
            ; Inline rather than through dma_run: this job rewrites the stack
            ; page, so no return address may be waiting on it.
            lda     #.byte1 load_low
            sta     0xd701
            lda     #0
            sta     0xd702
            sta     0xd704
            lda     #.byte0 load_low
            sta     0xd705
            ; The interpreter carries on after the SYS at the last line's 0,
            ; and the link after it is 0 too: the end of the program, READY.
            lda     #.byte0 (TEXT_END - 3)
            sta     zp:TXTPTR
            lda     #.byte1 (TEXT_END - 3)
            sta     zp:TXTPTR+1
            lda     #.byte0 TEXT_END
            sta     zp:TEXT_TOP
            lda     #.byte1 TEXT_END
            sta     zp:TEXT_TOP+1
            ldx     #keys_end - keys - 1
1$:         lda     keys,x
            sta     KEY_QUEUE,x
            dex
            bpl     1$
            lda     #keys_end - keys
            sta     zp:KEY_COUNT
            ldx     _InitialStack
            txs
            cli
            rts                     ; into BASIC's SYS, which prints READY.

; 10 PRINT "MERRY CHRISTMAS" / 20 GOTO 10, tokenized, each line's link
; absolute: PRINT is $99, GOTO $89. A real program, which RUN runs.
program:    .word   TEXT + 24
            .word   10
            .byte   0x99, " ", 0x22, "MERRY CHRISTMAS", 0x22, 0
            .word   TEXT + 33
            .word   20
            .byte   0x89, " 10", 0
            .word   0
program_end:

; CLR, then LIST and RETURN.
keys:       .byte   147, "LIST", 13
keys_end:

; DMA job list at A:X (high:low), which must be in bank 0.
dma_run:    sta     0xd701
            lda     #0
            sta     0xd702
            sta     0xd704
            stx     0xd705          ; enhanced list; the job runs here
            rts

; Where the snapshot lives, in attic RAM above the loader's staging (see
; src/basic.c, which patches it, and CLAUDE.md's memory map).
SNAP_MB:    .equ    0x87
SNAP_BANK1: .equ    0x00            ; $8700000: bank 1's 64 KB
SNAP_LOW:   .equ    0x01            ; $8710000: $0002-$1FFF

; F018B jobs with the option bytes dma.c uses: megabytes, a destination step
; of one, then copy, count, source, destination.
job:        .macro  smb, dmb, count, src, sbank, dst, dbank
            .byte   0x0b, 0x80, \smb, 0x81, \dmb, 0x85, 1, 0
            .byte   0
            .word   \count, \src
            .byte   \sbank
            .word   \dst
            .byte   \dbank, 0
            .word   0
            .endm

; $0000 and $0001 are the CPU port, not RAM, and are left out. A count of 0
; is 64 KB.
save_low:   job     0, SNAP_MB, 0x1ffe, 0x0002, 0, 0x0002, SNAP_LOW
save_bank1: job     0, SNAP_MB, 0, 0x0000, 1, 0x0000, SNAP_BANK1
load_low:   job     SNAP_MB, 0, 0x1ffe, 0x0002, SNAP_LOW, 0x0002, 0
load_bank1: job     SNAP_MB, 0, 0, 0x0000, SNAP_BANK1, 0x0000, 1
load_text:  job     0, 0, program_end - program, program, 0, TEXT, 0
