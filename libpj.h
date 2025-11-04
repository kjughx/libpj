#ifndef _LIBPJ_H_
#define _LIBPJ_H_

#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>

/* Temporary buffer */
#define __TMP_BUF_LEN 1024
static const char __buf[__TMP_BUF_LEN];

/* Start: Useful macros */
#define expect(cond) do {                              \
    if (!(cond)) {                               \
    printf("%s:%d: Expected %s, \n", __FILE__, __LINE__, #cond);       \
    }                                                         \
  } while(0);
#define expectf(cond, ...) _expectf(cond, __VA_ARGS__)
#define _expectf(cond, fmt, ...) \
  do {                                           \
    if (!(cond)) {                               \
    printf("%s:%d: Expected %s, " fmt"\n", __FILE__ , __LINE__, #cond, ##__VA_ARGS__);       \
    }                                                         \
  } while (0);

#define TODO() do { \
    fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, __func__);               \
    abort();                                                            \
  } while(0);

#define UNUSED(x) ((void)x);

/* End: Useful macros */

/* Start: DYNAMIC ARRAY */

/*
 `da` must begin with:
   ```
   struct Vec {
       <type> *items;
       size_t count;
       size_t capacity;
   };
   ```

   In da_init we allocate da->items. In the beginning of items we store the sizes of
   the elements in case for future improvements.
*/
#define da_decl(name, type) \
  typedef struct {          \
    type *items;            \
    size_t count;           \
    size_t capacity;        \
  } name;

#define da_foreach(da, item) for(size_t __i = 0; __i < (da)->count && ((item = (da)->items[__i]) || 1); ++__i)

#define __item_size(da) sizeof((da)->items[0])
#define __item_type(da) typeof((da)->items[0])

#define da_reserve(da, size) do {                                       \
    if ((da)->capacity < size) {                                        \
      if ((da)->items) {                                                \
        (da)->items = realloc((da)->items, (size) * __item_size((da)));              \
      } else {                                                          \
        (da)->items = malloc((size) * __item_size((da)));               \
      }                                                                 \
      assert((da)->items);                                              \
      (da)->count = 0;                                                  \
      (da)->capacity = (size);                                          \
    }                                                                   \
  } while(0);                                                           \

#ifndef UNIT_TEST
#define __INIT_CAP 256
#endif // UNIT_TEST
#define __MAX_ITEM_SIZE 256
#define da_init(da) do {                                        \
    if (!(da)->items) {                                         \
      da_reserve((da), __INIT_CAP);                             \
    }                                                           \
  } while(0);

#define __GROWTH_RATE 2
#define da_grow(da) do {                                                \
    (da)->items = realloc((da)->items,                                  \
                          (da)->capacity                                \
                          * __GROWTH_RATE                               \
                          * __item_size((da)));                         \
    assert((da)->items);                                                \
    (da)->capacity *= __GROWTH_RATE;                                    \
  } while(0);                                                           \

#define da_append(da, x) do {                               \
    da_init((da));                                          \
    if ((da)->count == (da)->capacity) {                    \
      da_grow((da));                                        \
    }                                                       \
    (da)->items[(da)->count++] = (x);                       \
  } while(0);


/* End: DYNAMIC ARRAY */

/* Start: Box */
#define Box(x) (typeof(x)) __box(x, sizeof(x))
static inline void* __box(void* x, size_t s) {
  void *p = malloc(s);
  memcpy(x, p, s);
  return p;
}

/* End: Box */

/* Start: STRING BUILDER */

/* A string builder is like a dynamic array specialized on strings */
typedef struct {
  char* items;
  size_t count;
  size_t capacity;
} String_Builder;

#define sb_append(sb, str) do {                      \
    size_t __l = strlen(str);                        \
    if ((sb)->count == 0) da_append(sb, '\0');       \
    (sb)->count--;                                     \
    for (size_t __i = 0; __i < __l; ++__i) {         \
      da_append((sb), str[__i]);                 \
    }                                                \
    da_append((sb), '\0');                           \
  } while(0);

#define sb_appends(sb, ...) __sb_appends((sb), __VA_ARGS__, NULL)
static inline void __sb_appends(String_Builder *sb, ...) {
  char *s;
  va_list ap;
  va_start(ap, sb);
  s = va_arg(ap, char*);
  while(s) {
    sb_append(sb, s);
    s = va_arg(ap, char*);
  }
}

#define sb_appendf(sb, fmt, ...) do {                                 \
  size_t __l = strlen(fmt);                                           \
  assert(__l < __TMP_BUF_LEN && "Too long format string");            \
  size_t __s = snprintf(__buf, __TMP_BUF_LEN, fmt, __VA_ARGS__);      \
  sb_append((sb), __buf);                                             \
} while(0);

static inline void __sb_read_file_fp(String_Builder *sb, FILE *fp) {
  long s;
  expectf(fseek(fp, 0, SEEK_END) == 0, "%s", strerror(errno));
  s = ftell(fp);
  expectf(s >= 0, "%s", strerror(errno));
  rewind(fp);
  da_reserve(sb, (size_t)s);
  s = fread(sb->items, 1, s, fp);
  expectf(s >= 0, "%s", strerror(errno));
  sb->count = s;
}

static inline void __sb_read_file_fd(String_Builder *sb, int fd) {
  expectf(fd > 0, "%s", "Invalid file descriptor");
  FILE *fp = fdopen(fd, "r");
  expectf(fp != NULL, "%s", strerror(errno));
  __sb_read_file_fp(sb, fp);
}

static inline void __sb_read_file_char(String_Builder *sb, const char* filename) {
  FILE *fp = fopen(filename, "r");
  expectf(fp != NULL, "%s", strerror(errno));
  __sb_read_file_fp(sb, fp);
  fclose(fp);
}

#define sb_read_file(sb, file) do {                            \
    _Generic((file),                                           \
             char*: __sb_read_file_char,                       \
             FILE*: __sb_read_file_fp,                         \
             int: __sb_read_file_fd                            \
             )((sb), (file));                                  \
  } while(0);

typedef struct {
  String_Builder* sb;
  char* start;
  char* end;
} String_View;

/* static inline String_View sb_view(String_Builder *sb,  */
#define sb_find(sb, p) \
  _Generic((p),                                      \
           char: sb_find_char,                        \
           int: sb_find_char,                        \
           char*: sb_find_str                        \
           )((sb), p);                               \

static inline String_View sb_find_char(String_Builder *sb, char c) {
  String_View sv = {.sb = sb};

  for (size_t i = 0; i < sb->count; ++i) {
    if (sb->items[i] == c) {
      sv.start = sv.end = &sb->items[i];
      break;
    }
  }

  return sv;
}

static inline String_View sb_find_str(String_Builder *sb, const char* s) {
  String_View sv = {.sb = sb };

  size_t __l = strlen(s);
  for (size_t i = 0; i < sb->count; ++i) {
    if (sb->items[i] == s[0] && i + __l < sb->count) {
      if (strncmp(&sb->items[i], s, __l) == 0) {
        sv.start = &sb->items[i];
        sv.end = (&sb->items[i] + __l);
      }
    }
  }

  return sv;
}

static inline String_Builder sv_to_sb(String_View sv) {
  String_Builder sb = {0};
  size_t __l = ((size_t)sv.end - (size_t)sv.start);
  da_reserve(&sb, __l);
  memcpy(sb.items, sv.start, __l);
  sb.count = __l;
  da_append(&sb, '\0');

  return sb;
}

typedef struct {
  String_View *items;
  size_t count;
  size_t capacity;

  bool is_c_delim;
  union {
    char c_delim;
    char *s_delim;
  };
} String_Split;

static inline String_Split _sb_split_char(String_Builder *sb, char c) {
  expect(sb);

  String_Builder tmp = *sb;
  String_Split sp = {0};
  for (;;) {
    String_View sv = sb_find(&tmp, c);

    /* Not found */
    if (!sv.start) {
      break;
    }

    sv.start = tmp.items; /* Start of current string */
    /* sv.end is equal to previous sv.start which points to @c */

    tmp.items = sv.end + 1; /* 1 past @c */
    tmp.count -= (sv.end - sv.start);
    da_append(&sp, sv);
  }

  String_View sv = {
    .sb = sb,
    .start = tmp.items,
    .end = &sb->items[sb->count - 1],
  };

  da_append(&sp, sv);

  return sp;
}

static inline String_Split _sb_split_str(String_Builder *sb, char *s) {
  String_Split sp = {0};
  TODO();

  UNUSED(sb);
  UNUSED(s);

  return sp;
}

#define sb_split(sb, c) \
  _Generic((c),                                \
           int: _sb_split_char,                \
           char*: _sb_split_str                 \
           )((sb), (c));


/* End: STRING BUILDER */

#endif // _LIBPJ_H_
