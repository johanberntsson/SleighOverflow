; The blitter's inner loop: one DMA job per column of a bitmap.
;
; gfx_blit in src/gfx.c clips, works out where the first column starts in
; attic RAM and in the framebuffer, and leaves it in gfx_col_*; this patches
; the job's source and destination for each column, triggers it, and steps
; both on. In C the same loop cost about 560 cycles a column, which on the
; 400-odd columns of a frame was more than the DMA itself.
;
; The CPU is halted while a job runs, so nothing here waits for one.

            .extern gfx_job
            .extern gfx_col_src, gfx_col_h, gfx_col_dst, gfx_col_sub, gfx_col_n

; Offsets into gfx_job: keep in step with the struct in src/gfx.c.
JOB_SRC_MB:   .equ 2
JOB_SRC:      .equ 14
JOB_SRC_BANK: .equ 16
JOB_DST:      .equ 17

; From one strip's last pixel column to the next strip's first: FB_STRIDE in
; src/vic4.h, less the seven columns already stepped along this one.
NEXT_STRIP:   .equ 1600 - 7

            .section code,text
            .public gfx_columns

; gfx_col_n columns, at least one.
gfx_columns:
            ; The job list is in bank 0 of megabyte 0.
            lda     #0
            sta     0xd702
            sta     0xd704
            ldx     gfx_col_sub

loop$:      lda     gfx_col_src
            sta     gfx_job+JOB_SRC
            lda     gfx_col_src+1
            sta     gfx_job+JOB_SRC+1
            lda     gfx_col_src+2
            and     #0x0f
            sta     gfx_job+JOB_SRC_BANK
            ; Megabyte: address bits 20-27, half from each of the top bytes.
            lda     gfx_col_src+3
            asl     a
            asl     a
            asl     a
            asl     a
            sta     gfx_job+JOB_SRC_MB
            lda     gfx_col_src+2
            lsr     a
            lsr     a
            lsr     a
            lsr     a
            ora     gfx_job+JOB_SRC_MB
            sta     gfx_job+JOB_SRC_MB
            lda     gfx_col_dst
            sta     gfx_job+JOB_DST
            lda     gfx_col_dst+1
            sta     gfx_job+JOB_DST+1
            ; **Both halves of the list address, every job.** The DMA steps
            ; that register along the list as it reads it, so after one job
            ; it points past this one -- and gfx_job straddled a page when
            ; this was found, so writing only the low byte, which is the
            ; trigger, sent the second job off into whatever followed.
            lda     #.byte1 gfx_job
            sta     0xd701
            lda     #.byte0 gfx_job
            sta     0xd705

            ; The next column of the bitmap is h bytes on.
            clc
            lda     gfx_col_src
            adc     gfx_col_h
            sta     gfx_col_src
            bcc     stepped$
            inc     gfx_col_src+1
            bne     stepped$
            inc     gfx_col_src+2
            bne     stepped$
            inc     gfx_col_src+3

            ; And the next pixel column of the screen is one byte along the
            ; character strip, or the first byte of the next strip.
stepped$:   inx
            cpx     #8
            beq     strip$
            inc     gfx_col_dst
            bne     count$
            inc     gfx_col_dst+1
            bra     count$
strip$:     ldx     #0
            clc
            lda     gfx_col_dst
            adc     #.byte0 NEXT_STRIP
            sta     gfx_col_dst
            lda     gfx_col_dst+1
            adc     #.byte1 NEXT_STRIP
            sta     gfx_col_dst+1

count$:     lda     gfx_col_n
            bne     low$
            dec     gfx_col_n+1
low$:       dec     gfx_col_n
            lda     gfx_col_n
            ora     gfx_col_n+1
            beq     done$
            jmp     loop$
done$:      rts
