TARGET   = --target=mega65
# BENCH=1 measures the DMA on the machine before the demo starts and prints a
# table: see src/bench.c.
BENCH   ?= 0
# STATS=1 prints every frame's draw time when F1 is pressed: see src/stats.c.
STATS   ?= 0
# TIMING=1 turns the border into a timing bar: see src/dos.c.
TIMING  ?= 0
# AUTOPLAY=n plays the first n steps of a scripted walkthrough, for headless
# xemu runs: see src/input.c. 0 is the keyboard.
AUTOPLAY ?= 0
CFLAGS   = $(TARGET) -O2 --speed -Isrc -I$(BUILD) -DBENCH=$(BENCH) \
           -DSTATS=$(STATS) -DTIMING=$(TIMING) -DAUTOPLAY=$(AUTOPLAY)
ASFLAGS  = $(TARGET)
LDFLAGS  = $(TARGET) --output-format=prg
# The toolchain's stock memory map: $2001-$9FFF for the program, with a C65
# BASIC stub in front of it. Resolved from Calypsi's linker-rules directory.
LINKFILE = mega65-plain.scm
CSTACK   = 1024

BUILD    = build
ASSETS   = assets
# Stranded's art, font and tune stand in for anything assets/ does not have
# yet: tools/convgfx.py takes each file from the first directory that has it.
# They are never copied in here; this repository holds only its own art.
PLACEHOLDERS ?= ../MegaStranded/assets
ASSET_DIRS = $(ASSETS) $(wildcard $(PLACEHOLDERS))

SRCS     = $(wildcard src/*.c)
ASRCS    = $(wildcard src/*.s)
OBJS     = $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS)) \
           $(patsubst src/%.s,$(BUILD)/%.o,$(ASRCS)) \
           $(BUILD)/gfx_table.o $(BUILD)/tables.o $(BUILD)/music_table.o

ELF      = $(BUILD)/sleigh.elf
# The MEGA65 ROM autoboots a file called autoboot.c65 and nothing else.
PRG      = $(BUILD)/autoboot.c65
D81      = $(BUILD)/sleigh.d81
DISK_NAME = SLEIGH OVERFLOW

# tools/convgfx.py writes every GFXn.BIN in one run, along with the table
# that says where each one decrunches to. Lower case on the host;
# tools/diskutil.rb names the file on the disk after it, in PETSCII.
GFX_RES  = $(foreach n,0 1 2 3 4 5 6,$(BUILD)/gfx$(n).bin)
GFX_GEN  = $(BUILD)/gfx_table.c $(BUILD)/gfx_ids.h
TABLES   = $(BUILD)/tables.c $(BUILD)/tables.h
# The tune, arranged for the SIDs: see tools/convmidi.py. assets/tune.mid
# when there is one, Stranded's until then.
TUNE     = $(firstword $(wildcard $(ASSETS)/tune.mid) \
                       $(PLACEHOLDERS)/stranded.mid)
MUSIC_RES = $(BUILD)/music.bin
MUSIC_GEN = $(BUILD)/music_table.c
RES      = $(GFX_RES) $(MUSIC_RES)

all: $(D81)

run: $(D81)
	xemu-xmega65 -besure -8 $(D81)

$(BUILD):
	mkdir -p $(BUILD)

# Changing a build flag has to force a rebuild: objects that do not depend
# on the Makefile silently keep the old flag, and the measurement comes back
# looking like the flag doing nothing (../SearchAndRescue learned this).
CONFIG_STAMP = $(BUILD)/config-$(CSTACK)-$(BENCH)-$(STATS)-$(TIMING)-$(AUTOPLAY).stamp

$(CONFIG_STAMP): Makefile | $(BUILD)
	rm -f $(BUILD)/config-*.stamp
	touch $@

# The labels are read out of the game's source, so it is an input too.
# A picture is name.png (painted in Krita and exported) or name.gif.
$(GFX_RES) $(GFX_GEN) &: tools/convgfx.py tools/labels.py src/game.c \
                         $(wildcard $(ASSETS)/*.png) \
                         $(foreach d,$(ASSET_DIRS),$(wildcard $(d)/*.gif $(d)/*.fnt)) \
                         | $(BUILD)
	python3 tools/convgfx.py $(BUILD) src/game.c $(ASSET_DIRS)

# Every object depends on every header, generated ones included. Crude, but
# a stale object built against a changed memory map fails in ways that look
# like hardware bugs.
HEADERS = $(wildcard src/*.h) $(BUILD)/gfx_ids.h $(BUILD)/tables.h

$(BUILD)/%.o: src/%.c $(HEADERS) $(CONFIG_STAMP) | $(BUILD)
	cc6502 $(CFLAGS) -c -o $@ $<

$(BUILD)/gfx_table.o: $(BUILD)/gfx_table.c $(HEADERS) $(CONFIG_STAMP)
	cc6502 $(CFLAGS) -c -o $@ $<

# The easing curves, as the integers they produce: see tools/convtables.py.
$(TABLES) &: tools/convtables.py | $(BUILD)
	python3 tools/convtables.py $(BUILD)

$(BUILD)/tables.o: $(BUILD)/tables.c $(HEADERS) $(CONFIG_STAMP)
	cc6502 $(CFLAGS) -c -o $@ $<

$(MUSIC_RES) $(MUSIC_GEN) &: tools/convmidi.py tools/convgfx.py \
                             $(TUNE) | $(BUILD)
	python3 tools/convmidi.py $(TUNE) $(BUILD)

$(BUILD)/music_table.o: $(BUILD)/music_table.c $(HEADERS) $(CONFIG_STAMP)
	cc6502 $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: src/%.s $(HEADERS) $(CONFIG_STAMP) | $(BUILD)
	as6502 $(ASFLAGS) -o $@ $<

# The list file is the memory map; take addresses from it, not from a
# hand-run link, because the object order decides where everything lands.
$(ELF): $(OBJS)
	ln6502 $(LDFLAGS) --cstack-size $(CSTACK) \
	    --list-file $(BUILD)/sleigh.lst -o $@ $(LINKFILE) $(OBJS)

# ln6502 -o names the ELF and writes the PRG beside it under the same stem.
$(PRG): $(ELF)
	cp $(BUILD)/sleigh.prg $@

# tools/diskutil.rb refuses to overwrite a file already on the image, so the
# image is always built from scratch.
$(D81): $(PRG) $(RES)
	rm -f $@
	ruby tools/diskutil.rb $@ -name "$(DISK_NAME)" -id so \
	    -writeprg -copyf1 $(PRG) \
	    -writeseq -copyf1 $(RES)

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
