/*
 * slot. superchis: the few C runtime functions the freestanding payload needs.  The compiler may
 * emit calls to memcpy/memset/memmove/memcmp even with -ffreestanding, and the shared pure modules
 * (slot_scan.c, slot_settings.c) call the usual string functions; the payload links no libc
 * (-nostdlib), so they are provided here.  Not hot: pixel loops have their own code.
 * Integer division/64-bit multiply come from libgcc (see the Makefile).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t n) {
  unsigned char *d = dst; const unsigned char *s = src;
  while (n--) *d++ = *s++;
  return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = dst; const unsigned char *s = src;
  if (d < s) { while (n--) *d++ = *s++; }
  else { d += n; s += n; while (n--) *--d = *--s; }
  return dst;
}

void *memset(void *dst, int c, size_t n) {
  unsigned char *d = dst;
  while (n--) *d++ = (unsigned char)c;
  return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
  const unsigned char *x = a, *y = b;
  for (; n; n--, x++, y++)
    if (*x != *y) return *x < *y ? -1 : 1;
  return 0;
}

size_t strlen(const char *s) {
  const char *p = s;
  while (*p) p++;
  return (size_t)(p - s);
}

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b) { a++; b++; }
  return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
  for (; n; n--, a++, b++) {
    if (*a != *b) return (unsigned char)*a - (unsigned char)*b;
    if (!*a) return 0;
  }
  return 0;
}

char *strchr(const char *s, int c) {
  for (;; s++) {
    if (*s == (char)c) return (char *)s;
    if (!*s) return 0;
  }
}

char *strrchr(const char *s, int c) {
  const char *last = 0;
  for (;; s++) {
    if (*s == (char)c) last = s;
    if (!*s) return (char *)last;
  }
}

size_t strcspn(const char *s, const char *reject) {
  size_t n = 0;
  for (; s[n]; n++)
    for (const char *r = reject; *r; r++)
      if (s[n] == *r) return n;
  return n;
}

/* libgcc's division helpers call these on a divide by zero; the payload never divides by a
 * value that can be zero, and returning keeps the link free of libc's raise(). */
int __aeabi_idiv0(int r) { return r; }
long long __aeabi_ldiv0(long long r) { return r; }
