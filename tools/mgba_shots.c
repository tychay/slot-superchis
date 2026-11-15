// slot: headless mGBA runner for the preview/screenshot harness (spike 2.1).
// Written for this fork (GPL-3.0-or-later); links against libmgba (MPL-2.0).
//
// usage: mgba_shots <rom.gba> <outdir> <sequence>
//   sequence: comma-separated steps, see tools/preview_shots.sh for the grammar.
// Runs with no window, no audio, no GUI permissions; frames are emulated as fast as the
// host allows and PNGs are written with libmgba's own PNG writer.
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <mgba-util/vfs.h>
#include <stdarg.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct { const char *name; int bit; } keymap[] = {
  {"A", 0}, {"B", 1}, {"Select", 2}, {"Start", 3}, {"Right", 4},
  {"Left", 5}, {"Up", 6}, {"Down", 7}, {"R", 8}, {"L", 9},
};

static void quiet_log(struct mLogger *l, int cat, enum mLogLevel lvl, const char *fmt, va_list args) { (void)l; (void)cat; (void)lvl; (void)fmt; (void)args; }
static struct mLogger quiet = { .log = quiet_log };

static struct mCore *core;
static unsigned frame;
static unsigned keys;

static void run_frames(unsigned n) {
  while (n--) { core->setKeys(core, keys); core->runFrame(core); frame++; }
}

static int shot(const char *outdir, const char *name) {
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s.png", outdir, name);
  struct VFile *vf = VFileOpen(path, O_WRONLY | O_CREAT | O_TRUNC);
  if (!vf) { fprintf(stderr, "cannot open %s\n", path); return 1; }
  bool ok = mCoreTakeScreenshotVF(core, vf);
  vf->close(vf);
  if (!ok) { fprintf(stderr, "screenshot failed: %s\n", path); return 1; }
  printf("%s (frame %u)\n", path, frame);
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 4) { fprintf(stderr, "usage: %s rom.gba outdir sequence\n", argv[0]); return 2; }
  mLogSetDefaultLogger(&quiet);   // silence "Bad cartridge Store16" etc. (SuperChis mode-reg writes)
  core = mCoreFind(argv[1]);
  if (!core) { fprintf(stderr, "not a ROM mGBA understands: %s\n", argv[1]); return 2; }
  core->init(core);
  mCoreInitConfig(core, NULL);
  unsigned w, h;
  core->desiredVideoDimensions(core, &w, &h);
  color_t *vbuf = calloc((size_t)w * h, sizeof(color_t));
  core->setVideoBuffer(core, vbuf, w);
  core->setAudioBufferSize(core, 1024);
  if (!mCoreLoadFile(core, argv[1])) { fprintf(stderr, "cannot load %s\n", argv[1]); return 2; }
  mCoreConfigSetDefaultValue(&core->config, "idleOptimization", "ignore");
  core->reset(core);

  char *seq = strdup(argv[3]), *save = NULL;
  unsigned shotn = 0, rc = 0;
  for (char *tok = strtok_r(seq, ",", &save); tok && !rc; tok = strtok_r(NULL, ",", &save)) {
    char name[256];
    if (!strncmp(tok, "wait:", 5)) {
      run_frames(atoi(tok + 5));
    } else if (!strncmp(tok, "hold:", 5)) {            // hold:KEY[:frames], default 10
      char *k = tok + 5, *c = strchr(k, ':');
      unsigned n = c ? atoi(c + 1) : 10;
      if (c) *c = 0;
      int found = 0;
      for (unsigned i = 0; i < sizeof(keymap) / sizeof(*keymap); i++)
        if (!strcasecmp(k, keymap[i].name)) { keys |= 1u << keymap[i].bit; run_frames(n); keys &= ~(1u << keymap[i].bit); found = 1; }
      if (!found) { fprintf(stderr, "unknown key %s\n", k); rc = 2; }
    } else if (!strncmp(tok, "shot", 4)) {              // shot or shot:name
      if (tok[4] == ':') snprintf(name, sizeof(name), "%s", tok + 5);
      else snprintf(name, sizeof(name), "step%02u_f%u", shotn, frame);
      shotn++;
      rc = shot(argv[2], name);
    } else {                                            // bare key name: tap (4 frames down, 6 up)
      int found = 0;
      for (unsigned i = 0; i < sizeof(keymap) / sizeof(*keymap); i++)
        if (!strcasecmp(tok, keymap[i].name)) {
          keys |= 1u << keymap[i].bit; run_frames(4);
          keys &= ~(1u << keymap[i].bit); run_frames(6); found = 1;
        }
      if (!found) { fprintf(stderr, "unknown step '%s'\n", tok); rc = 2; }
    }
  }
  free(seq);
  core->deinit(core);
  free(vbuf);
  return rc;
}
