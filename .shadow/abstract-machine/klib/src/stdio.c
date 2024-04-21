#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)


//bufsize 256

void reverse(char *s) {
  char *p = s;
  while (*p) {
    p++;
  }
  p--;
  while (s < p) {
    char t = *s;
    *s = *p;
    *p = t;
    s++;
    p--;
  }
}

void itoa(int n, char *s, int base) {
  int i = 0;
  int sign = n;
  if (sign < 0) {
    n = -n;
  }
  do {
    int digit = n % base;
    s[i++] = digit < 10 ? digit + '0' : digit - 10 + 'a';
  } while ((n /= base) > 0);
  if (sign < 0) {
    s[i++] = '-';
  }
  s[i] = '\0';
  reverse(s);
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(NULL, fmt, ap);
  va_end(ap);
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  int ret = 0;
  char buf[256];
  char *p;
  for (p = (char *)fmt; *p; p++) {
    if (*p != '%') {
      if (out) {
        *out++ = *p;
      }
      ret++;
      continue;
    }
    p++;
    switch (*p) {
      case 's': {
        const char *str = va_arg(ap, const char *);
        if (out) {
          while (*str) {
            *out++ = *str++;
          }
        }
        ret += strlen(str);
        break;
      }
      case 'd': {
        int num = va_arg(ap, int);
        itoa(num, buf, 10);
        if (out) {
          char *str = buf;
          while (*str) {
            *out++ = *str++;
          }
        }
        ret += strlen(buf);
        break;
      }
      case 'x': {
        int num = va_arg(ap, int);
        itoa(num, buf, 16);
        if (out) {
          char *str = buf;
          while (*str) {
            *out++ = *str++;
          }
        }
        ret += strlen(buf);
        break;
      }
      default:
        if (out) {
          *out++ = *p;
        }
        ret++;
        break;
    }
  }
  if (out) {
    *out = '\0';
  }
  return ret;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  int ret = 0;
  char buf[256];
  char *p;
  for (p = (char *)fmt; *p; p++) {
    if (*p != '%') {
      if (out) {
        *out++ = *p;
      }
      ret++;
      continue;
    }
    p++;
    switch (*p) {
      case 's': {
        const char *str = va_arg(ap, const char *);
        if (out) {
          while (*str && n > 1) {
            *out++ = *str++;
            n--;
          }
        }
        ret += strlen(str);
        break;
      }
      case 'd': {
        int num = va_arg(ap, int);
        itoa(num, buf, 10);
        if (out) {
          char *str = buf;
          while (*str && n > 1) {
            *out++ = *str++;
            n--;
          }
        }
        ret += strlen(buf);
        break;
      }
      case 'x': {
        int num = va_arg(ap, int);
        itoa(num, buf, 16);
        if (out) {
          char *str = buf;
          while (*str && n > 1) {
            *out++ = *str++;
            n--;
          }
        }
        ret += strlen(buf);
        break;
      }
      default:
        if (out) {
          *out++ = *p;
        }
        ret++;
        break;
    }
  }
  if (out && n > 0) {
    *out = '\0';
  }
  return ret;
}

#endif
