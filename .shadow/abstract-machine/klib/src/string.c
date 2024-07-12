#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)


size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) {
  char *ret = dst;
  while ((*dst++ = *src++) != '\0')
    ;
  return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *ret = dst;
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }
  for (; i < n; i++) {
    dst[i] = '\0';
  }
  return ret;
}

char *strcat(char *dst, const char *src) {
  char *ret = dst;
  while (*dst != '\0') {
    dst++;
  }
  while ((*dst++ = *src++) != '\0')
    ;
  return ret;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t i;
  for (i = 0; i < n && s1[i] == s2[i] && s1[i] != '\0'; i++)
    ;
  if (i == n) {
    return 0;
  }
  return s1[i] - s2[i];
}

int strcmp(const char *s1, const char *s2) {
   int ret=0;
  while(*s1 && *s2){
    if(*s1!=*s2){
      ret=*s1-*s2;
      break;
    }
    s1++;
    s2++;
  }
  if(*s1 && !*s2){
    ret=1;
  }
  else if(!*s1 && *s2){
    ret=-1;
  }
  return ret;
 
}


void *memset(void *s, int c, size_t n) {
  unsigned char *p = s;
  while (n-- > 0) {
    *p++ = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *pdst = dst;
  const unsigned char *psrc = src;
  if (pdst < psrc) {
    while (n-- > 0) {
      *pdst++ = *psrc++;
    }
  } else if (pdst > psrc) {
    pdst += n;
    psrc += n;
    while (n-- > 0) {
      *--pdst = *--psrc;
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *pout = out;
  const unsigned char *pin = in;
  while (n-- > 0) {
    *pout++ = *pin++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2;
  while (n-- > 0) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

#endif
