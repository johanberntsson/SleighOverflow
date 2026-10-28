#include <calypsi/stubs.h>
#include <fcntl.h>

#include "dma.h"
#include "exo.h"
#include "gfx.h"
#include "loader.h"

#if GFX_END > ATTIC_MUSIC
#error "the bitmaps have grown into the music"
#endif

// Kernal reads land in the 64K the program can see, so every resource is
// bounced through here and DMAd up to attic RAM a chunk at a time.
#define CHUNK 512
static uint8_t staging[CHUNK];

// Called every PROGRESS_CHUNKS chunks read, across all the files.
#define PROGRESS_CHUNKS 32  // 16 KB
static void (*progress)(void);
static uint8_t chunks;

static const char *failure;
static const char *failed_file;

static int fail(const char *why, const char *name)
{
  failure = why;
  failed_file = name;
  return -1;
}

const char *loader_error(void)
{
  return failure;
}

const char *loader_error_file(void)
{
  return failed_file;
}

// Nothing promises that one Kernal read delivers everything asked for.
static uint16_t read_exact(int fd, uint8_t *dest, uint16_t length)
{
  uint16_t left = length;

  while (left) {
    size_t got = _Stub_read(fd, dest, left);

    if (!got || got > left)
      break;
    dest += got;
    left -= (uint16_t)got;
  }
  return length - left;
}

// Each file is its crunched length, then an exomizer stream, then padding:
// the Kernal reports EOF on a SEQ file 256 bytes early whatever its size, so
// nothing here looks for the end. Resources are SEQ, which is what
// _Stub_open's secondary address defaults to.
static int load_crunched(const char *name, uint32_t dest)
{
  int fd = _Stub_open(name, O_RDONLY | O_BINARY);
  uint32_t length, at = ATTIC_STAGE;

  if (fd < 0)
    return fail("CANNOT OPEN", name);
  if (read_exact(fd, (uint8_t *)&length, sizeof length) != sizeof length) {
    _Stub_close(fd);
    return fail("NO LENGTH IN", name);
  }
  while (length) {
    uint16_t want = length > CHUNK ? CHUNK : (uint16_t)length;

    if (read_exact(fd, staging, want) != want) {
      _Stub_close(fd);
      return fail("SHORT READ", name);
    }
    dma_copy((uint32_t)(uint16_t)staging, at, want);
    at += want;
    length -= want;
    if (progress && ++chunks == PROGRESS_CHUNKS) {
      chunks = 0;
      progress();
    }
  }
  _Stub_close(fd);
  exo_decrunch(ATTIC_STAGE, dest);
  return 0;
}

int load_resources(void (*report)(void))
{
  uint8_t i;

  progress = report;
  for (i = 0; i < GFX_FILES; i++)
    if (load_crunched(gfx_files[i].name, gfx_files[i].addr))
      return -1;
  // The tune, last: tools/convsong.py, src/music.c.
  return load_crunched("MUSIC.BIN", ATTIC_MUSIC);
}
