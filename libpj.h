#ifndef _LIBPJ_H_
#define _LIBPJ_H_

#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

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

#define da_reserve(da, size) do {                                  \
    assert(da && "NULL dynamic array");                            \
    static_assert(__item_size((da)) < __MAX_ITEM_SIZE);            \
    uint8_t *p = malloc(1 + (size) * __item_size((da)));           \
    assert(p);                                                     \
    *p = __item_size((da));                                        \
    (da)->items = (__item_type((da))*) (p + 1);                     \
    (da)->count = 0;                                               \
    (da)->capacity = (size);                                       \
  } while(0);                                                      \

#define __INIT_CAP 2
#define __MAX_ITEM_SIZE 256
#define da_init(da) do {                                        \
    assert(da && "NULL dynamic array");                         \
    if (!(da)->items) {                                         \
      da_reserve((da), __INIT_CAP);                             \
    }                                                           \
  } while(0);

#define __GROWTH_RATE 1.414
#define da_grow(da) do {                                                \
    assert(da && "NULL dynamic array");                                 \
    uint8_t *p = realloc((uint8_t*)(da)->items - 1,                     \
                         (da)->capacity                                 \
                         * __GROWTH_RATE                                \
                         * __item_size((da)) + 1);                      \
    assert(p);                                                          \
    (da)->items = (__item_type((da))*)(p + 1);                          \
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

/* Temporary buffer */
#define __TMP_BUF_LEN 1024
static char __buf[__TMP_BUF_LEN];

#define sb_append(sb, str) do {                   \
    size_t __l = strlen(str);                     \
    for (int __i = 0; __i < __l; ++__i) {         \
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

/* End: STRING BUILDER */

#endif // _LIBPJ_H_
