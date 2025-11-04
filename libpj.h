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

/* Start: STRING BUILDER */

/* A string builder is like a dynamic array specialized on strings */
typedef struct {
  char* items;
  size_t count;
  size_t capacity;
} String_Builder;

#define sb_append(sb, str) do {                   \
    size_t __l = strlen(str);                     \
    for (size_t __i = 0; __i < __l; ++__i) {         \
      da_append((sb), str[__i]);                  \
    }                                             \
  } while(0);                                     \

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
  __buf[__s] = '\0';                                                  \
  sb_append((sb), __buf);                                             \
} while(0);

static inline char* sb_dump(String_Builder *sb) {
  sb_append(sb, "\0");
  return sb->items;
}

/* End: STRING BUILDER */

#endif // _LIBPJ_H_
