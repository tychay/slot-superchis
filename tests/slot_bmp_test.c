/*
 * slot. superchis: host test for the Cart Studio art format (task 6.2).
 *
 * usage: slot_bmp_test <dir> [<dir> ...] [--games <Games dir>]
 * Each <dir> is a tree with SYSTEM/SLOT/{CARTS,CARTS_FLAT}/{GBA,GB,GBC}/<stem>.bmp as written by
 * vendor/slot-advance/tools/convert_labels.py (generated in the build image by tests/make_art_fixtures.py,
 * or converted real labels in tests/fixtures/art, flat layout CARTS/GBA/..).  Every cart BMP must pass
 * Slot_BMPValid for its shelf's size and decode row by row to the screen format.
 * With --games, every ROM name in <Games>/{GBA,GB,GBC} must resolve to an existing art file through
 * Slot_ArtPathFor (what the carousel opens), checked against the first <dir>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "slot/slot_format.h"

static unsigned failures, checked;
#define CHECK(c, ...) do { if (!(c)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static const char *const plats[3] = {"GBA", "GB", "GBC"};
static const unsigned pw[3] = {120, 104, 104}, ph[3] = {71, 112, 112};

static unsigned char *slurp(const char *path, unsigned *n) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
  unsigned char *b = malloc((size_t)len + 1);
  if (fread(b, 1, (size_t)len, f) != (size_t)len) { free(b); fclose(f); return NULL; }
  fclose(f); *n = (unsigned)len; return b;
}

/* Decode like Slot_PumpArt: row by row, bottom-up file order to top-down image. */
static void check_bmp(const char *path, unsigned plat) {
  unsigned n = 0, w = pw[plat], h = ph[plat];
  unsigned char *b = slurp(path, &n);
  CHECK(b != NULL, "cannot read %s", path);
  if (!b) return;
  CHECK(Slot_BMPValid(b, n, w, h), "%s: not a valid %ux%u cart BMP", path, w, h);
  if (Slot_BMPValid(b, n, w, h)) {
    uint16_t *img = calloc(w * h, 2), row[120];
    unsigned keys = 0, opaque = 0, bad = 0;
    for (unsigned r = 0; r < h; r++) {
      memcpy(row, b + 54 + r * w * 2, w * 2);
      Slot_DecodeBMP555Row(row, w);
      memcpy(img + (h - 1 - r) * w, row, w * 2);
    }
    for (unsigned i = 0; i < w * h; i++) {
      if (img[i] == 0x8000) keys++;
      else { opaque++; if (img[i] & 0x8000) bad++; }
    }
    CHECK(bad == 0, "%s: %u decoded pixels carry bit 15 but are not the key", path, bad);
    CHECK(keys > 0 && opaque > w * h / 4, "%s: implausible key/opaque counts %u/%u", path, keys, opaque);
    CHECK(img[0] == 0x8000, "%s: top-left corner should be transparent", path);
    free(img);
  }
  free(b);
  checked++;
}

static void walk(const char *root, const char *variant) {
  for (unsigned p = 0; p < 3; p++) {
    char dir[1024];
    snprintf(dir, sizeof dir, "%s/%s/%s", root, variant, plats[p]);
    DIR *d = opendir(dir);
    if (!d) continue;
    struct dirent *e;
    while ((e = readdir(d))) {
      size_t l = strlen(e->d_name);
      if (l < 5 || strcmp(e->d_name + l - 4, ".bmp") || e->d_name[0] == '.') continue;
      char path[2048];
      snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
      check_bmp(path, p);
    }
    closedir(d);
  }
}

static void check_games(const char *root, const char *games) {
  unsigned roms = 0, art = 0;
  for (unsigned p = 0; p < 3; p++) {
    char dir[1024];
    snprintf(dir, sizeof dir, "%s/%s", games, plats[p]);
    DIR *d = opendir(dir);
    if (!d) continue;
    struct dirent *e;
    while ((e = readdir(d))) {
      char out[300], full[2048];
      if (e->d_name[0] == '.') continue;
      if (!Slot_ArtPathFor(out, 256, 1, plats[p], e->d_name)) continue;
      roms++;
      snprintf(full, sizeof full, "%s%s", root, out);       /* out starts with /SYSTEM/SLOT/... */
      struct stat st;
      if (stat(full, &st) == 0) art++;
      else printf("no art for %s/%s -> %s\n", plats[p], e->d_name, out);
    }
    closedir(d);
  }
  printf("games: %u ROMs, %u with art\n", roms, art);
  CHECK(roms == art, "%u ROMs without art", roms - art);
}

int main(int argc, char **argv) {
  const char *games = NULL, *first = NULL;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--games") && i + 1 < argc) { games = argv[++i]; continue; }
    char root[1100];
    /* a tree is either <dir>/SYSTEM/SLOT/CARTS */
    snprintf(root, sizeof root, "%s/SYSTEM/SLOT", argv[i]);
    struct stat st;
    if (stat(root, &st)) snprintf(root, sizeof root, "%s", argv[i]);
    walk(root, "CARTS");
    walk(root, "CARTS_FLAT");
    if (!first) first = argv[i];
  }
  if (games && first) check_games(first, games);
  /* path rules independent of any card */
  char out[64];
  CHECK(Slot_ArtPathFor(out, sizeof out, 1, "GB", "Mario Bros. Deluxe (USA).gbc") && !strcmp(out, "/SYSTEM/SLOT/CARTS/GB/Mario Bros. Deluxe (USA).bmp"), "stem keeps inner dots: %s", out);
  CHECK(Slot_ArtPathFor(out, sizeof out, 0, "GBA", "x.GBA") && !strcmp(out, "/SYSTEM/SLOT/CARTS_FLAT/GBA/x.bmp"), "flat path: %s", out);
  CHECK(!Slot_ArtPathFor(out, sizeof out, 1, "GBA", "noextension"), "no extension must not resolve");
  CHECK(!Slot_ArtPathFor(out, 27, 1, "GBA", "a.gba"), "too long must not resolve");
  /* decoder: magenta key, channel swap */
  uint16_t px[3] = {0x7c1f, 0x001f, 0x7c00};
  Slot_DecodeBMP555Row(px, 3);
  CHECK(px[0] == 0x8000 && px[1] == 0x7c00 && px[2] == 0x001f, "decode: %04x %04x %04x", px[0], px[1], px[2]);
  printf("slot_bmp_test: %u BMPs checked, %u failures\n", checked, failures);
  CHECK(checked > 0, "no BMPs found: pass at least one fixture directory");
  return failures ? 1 : 0;
}
