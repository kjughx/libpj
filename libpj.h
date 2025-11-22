#ifndef _LIBPJ_H_
#define _LIBPJ_H_

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/* Temporary buffer */
#define __TMP_BUF_LEN 1024
static char __buf[__TMP_BUF_LEN] = {0};

/* Start: Useful macros */
#define expect(cond)                                                           \
  do {                                                                         \
    int c = (cond);                                                     \
    if (!c) {                                                             \
      printf("%s:%d: Expected `%s`, got %d\n", __FILE__, __LINE__, #cond, c);             \
    }                                                                          \
  } while (0);
#define expectf(cond, ...) _expectf(cond, __VA_ARGS__)
#define _expectf(cond, fmt, ...)                                               \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("%s:%d: Expected `%s`, " fmt "\n", __FILE__, __LINE__, #cond,       \
             ##__VA_ARGS__);                                                   \
    }                                                                          \
  } while (0);

#define TODO()                                                                 \
  do {                                                                         \
    fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, __func__);        \
    abort();                                                                   \
  } while (0);

#define UNUSED(x) ((void)x);

#define ARRAY_LEN(x) (sizeof((x)) / (sizeof(*(x))))

#define matches(s1, s2) ((strcmp((s1), (s2)) == 0))
#define matches_n(s1, s2, n) ((strncmp((s1), (s2), (n)) == 0))

#define swap(x, y) do { \
  _Static_assert(sizeof(x) == sizeof(y)); \
  typeof(x) __t = (y);                    \
  (y) = (x);                              \
  (x) = __t;                              \
} while(0);

static inline void __print_int(int x) { printf("%d\n", x); }
static inline void __print_str(char *x) { printf("%s\n", x); }
#define print(x) _Generic((x),                      \
                          int: __print_int,        \
                          char*: __print_str)(x);
#define println(fmt, ...) (printf(fmt"\n", __VA_ARGS__))

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

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

   In da_init we allocate da->items. In the beginning of items we store the
   sizes of the elements in case for future improvements.
*/
#define da_decl(name, type)                                                    \
  typedef struct {                                                             \
    type *items;                                                               \
    size_t count;                                                              \
    size_t capacity;                                                           \
  } name;

#define da_foreach(da, item)                                                   \
  for (size_t __i = 0; __i < (da)->count && ((item = (da)->items[__i]) || 1);  \
       ++__i)
#define da_foreach_ref(da, item)                                               \
  for (size_t __i = 0; __i < (da)->count && ((item = &(da)->items[__i]) || 1); \
       ++__i)

#define __item_size(da) sizeof((da)->items[0])
#define __item_type(da) typeof((da)->items[0])

#define da_reserve(da, size)                                                   \
  do {                                                                         \
    if ((da)->capacity < size) {                                               \
      if ((da)->items) {                                                       \
        (da)->items = realloc((da)->items, (size) * __item_size((da)));        \
      } else {                                                                 \
        (da)->items = malloc((size) * __item_size((da)));                      \
      }                                                                        \
      assert((da)->items);                                                     \
      (da)->count = 0;                                                         \
      (da)->capacity = (size);                                                 \
    }                                                                          \
  } while (0);

#ifndef UNIT_TEST
#define __INIT_CAP 256
#endif // UNIT_TEST
#define __MAX_ITEM_SIZE 256
#define da_init(da)                                                            \
  do {                                                                         \
    if (!(da)->items) {                                                        \
      da_reserve((da), __INIT_CAP);                                            \
    }                                                                          \
  } while (0);

#define __GROWTH_RATE 2
#define da_grow(da)                                                            \
  do {                                                                         \
    (da)->items = realloc((da)->items,                                         \
                          (da)->capacity * __GROWTH_RATE * __item_size((da))); \
    assert((da)->items);                                                       \
    (da)->capacity *= __GROWTH_RATE;                                           \
  } while (0);

#define da_append(da, x)                                                       \
  do {                                                                         \
    da_init((da));                                                             \
    if ((da)->count == (da)->capacity) {                                       \
      da_grow((da));                                                           \
    }                                                                          \
    (da)->items[(da)->count] = (x);                                          \
    (da)->count++;                                                      \
  } while (0);

#define da_map(da, f)                                                          \
  do {                                                                         \
    __item_type((da)) * __item;                                                \
    da_foreach_ref((da), __item) { *__item = f(*__item); }                     \
  } while (0);

#define da_sort(da, f) (qsort((da)->items, (da)->count, __item_size((da)), (f)))

/* End: DYNAMIC ARRAY */

/* Start: Box */
#define Box(x) __box(&x, sizeof((x)))
static inline void *__box(void *x, size_t s) {
  void *p = malloc(s);
  memcpy(p, x, s);
  return p;
}

/* End: Box */

/* Start: Linear Algebra */
/*
   A Matrix has the shape:
   struct {
     <type> *items;
     size_t nx;
     size_t ny;
   };

   A Vector has shape:
   struct {
     <type> *items;
     size_t n;
   };
*/
#define ma_size(ma) ((ma)->nx * (ma)->ny * sizeof((ma)->items[0]))

#define ma_init(ma)                                                            \
  do {                                                                         \
    if (!(ma)->items) {                                                        \
      (ma)->items = malloc(ma_size((ma)));                                     \
    }                                                                          \
    expect((ma)->items);                                                       \
  } while (0);

/* Returns pointer to element */
#define ma_at(ma, x, y) ((ma)->items + (ma)->nx * y + x)

#define ma_diag(ma, val)                                                       \
  do {                                                                         \
    expect((ma)->nx == (ma)->ny);                                              \
    ma_fill((ma), 0);                                                          \
    for (size_t __i = 0; __i < (ma)->nx; ++__i) {                              \
      *ma_at((ma), __i, __i) = val;                                            \
    }                                                                          \
  } while (0);

#define ma_fill(ma, val)                                                       \
  do {                                                                         \
    ma_init((ma));                                                             \
    memset((ma)->items, val, ma_size((ma)));                                   \
  } while (0);

#define ma_inbounds(ma, x, y) ((0 <= (x) && (x) < (typeof((x)))(ma)->nx) && (0 <= (y) && (y) < (typeof((y)))(ma)->ny))

#define v_size(v) ((v)->n * sizeof((v)->items[0]))
#define v_init(v)                                                              \
  do {                                                                         \
    if (!(v)->items) {                                                         \
      (v)->items = malloc(v_size(v));                                          \
    }                                                                          \
  } while (0);

#define v_fill(v, val)                                                         \
  do {                                                                         \
    memset((v)->items, val, v_size((v)));                                      \
  } while (0);

#define ma_mul(ma1, ma2) TODO()
#define ma_mulv(ma, v) TODO()

typedef struct {
  ssize_t x, y;
} Vector2;

typedef struct {
  float x, y;
} Vector2f;

/* End: Linear Algebra */

/* Start: STRING BUILDER */

/* A string builder is like a dynamic array specialized on strings */
typedef struct {
  char *items;
  size_t count;
  size_t capacity;
} String_Builder;

#define sb_append(sb, str)                                                     \
  do {                                                                         \
    size_t __l = strlen(str);                                                  \
    if ((sb)->count == 0)                                                      \
      da_append(sb, '\0');                                                     \
    (sb)->count--;                                                             \
    for (size_t __i = 0; __i < __l; ++__i) {                                   \
      da_append((sb), str[__i]);                                               \
    }                                                                          \
    da_append((sb), '\0');                                                     \
  } while (0);

#define sb_skip_word(sb) do { \
    while (*(sb)->items && isalnum(*(sb)->items)) { \
      (sb)->items++;                             \
      (sb)->count--;                             \
    }                                            \
    while (*(sb)->items && !isalnum(*(sb)->items)) { \
      (sb)->items++;                                \
      (sb)->count--;                                \
    }                                            \
  } while(0);

#define sb_appends(sb, ...) __sb_appends((sb), __VA_ARGS__, NULL)
static inline void __sb_appends(String_Builder *sb, ...) {
  char *s;
  va_list ap;
  va_start(ap, sb);
  s = va_arg(ap, char *);
  while (s) {
    sb_append(sb, s);
    s = va_arg(ap, char *);
  }
}

#define sb_appendf(sb, fmt, ...)                                               \
  do {                                                                         \
    size_t __l = strlen(fmt);                                                  \
    assert(__l < __TMP_BUF_LEN && "Too long format string");                   \
    size_t __s = snprintf(__buf, __TMP_BUF_LEN, fmt, __VA_ARGS__);             \
    sb_append((sb), __buf);                                                    \
  } while (0);

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

static inline void __sb_read_file_char(String_Builder *sb,
                                       const char *filename) {
  FILE *fp = fopen(filename, "r");
  expectf(fp != NULL, "%s", strerror(errno));
  __sb_read_file_fp(sb, fp);
  fclose(fp);
}

#define sb_read_file(sb, file)                                                 \
  do {                                                                         \
    _Generic((file),                                                           \
        char *: __sb_read_file_char,                                           \
        FILE *: __sb_read_file_fp,                                             \
        int: __sb_read_file_fd)((sb), (file));                                 \
  } while (0);

static inline void sb_strip(String_Builder *sb, char c) {
  if (sb->count == 0) return;
  if (sb->items[0] == c) {
    sb->items++; /* LEAK */
    sb->count--;
  }
  if (sb->items[sb->count - 1] == c) {
    sb->count--;
  }
}

typedef struct {
  const char *buf;
  size_t size;
} String_View;

#define sb_find(sb, p)                                                         \
  _Generic((p), char: sb_find_char, int: sb_find_char, char *: sb_find_str)(   \
      (sb), p)

static inline String_View sb_find_char(String_Builder *sb, char c) {
  String_View sv = {0};

  for (size_t i = 0; i < sb->count; ++i) {
    if (sb->items[i] == c) {
      sv.buf = &sb->items[i];
      sv.size = sb->count - (sv.buf - sb->items);
      break;
    }
  }

  return sv;
}

static inline String_View sb_find_str(String_Builder *sb, const char *s) {
  String_View sv = {0};

  size_t __l = strlen(s);
  for (size_t i = 0; i < sb->count; ++i) {
    if (sb->items[i] == s[0] && i + __l < sb->count) {
      if (strncmp(&sb->items[i], s, __l) == 0) {
        sv.buf = &sb->items[i];
        sv.size = sb->count - (sv.buf - sb->items);
        break;
      }
    }
  }

  return sv;
}

static inline String_Builder sv_to_sb(String_View sv) {
  String_Builder sb = {0};
  da_reserve(&sb, sv.size);
  memcpy(sb.items, sv.buf, sv.size);
  sb.count = sv.size;
  da_append(&sb, '\0');

  return sb;
}

static inline const char *sv_to_cstr(String_View sv) {
  return strndup(sv.buf, sv.size);
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
  expect(sb != NULL);

  String_Builder tmp = *sb;
  String_Split sp = {0};
  const char *p = tmp.items;
  for (;;) {
    String_View next = sb_find(&tmp, c);

    /* Not found */
    if (!next.buf) {
      break;
    }

    String_View sv = {
        .buf = p,
        .size = tmp.count - next.size,
    };

    /* Skip @c */
    next.buf++;
    next.size--;

    da_append(&sp, sv);
    tmp.items += (next.buf - p);
    tmp.count -= (next.buf - p);

    p = next.buf;
  }

  String_View sv = {
      .buf = tmp.items,
      .size = (sb->count - tmp.count),
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

#define sb_split(sb, c)                                                        \
  _Generic((c), int: _sb_split_char, char *: _sb_split_str)((sb), (c));

/* End: STRING BUILDER */

/* Start: Linked List */
typedef struct node {
  void *k;
  void *v;
  struct node *next;
  struct node *prev;
} node_t;

#define ll_append(ll, __k) do {                              \
    (ll)->next = Box((node_t){.k = (void*)__k});             \
  } while(0);

#define ll_foreach(ll, __key)                \
  for (node_t *__p = (ll); \
       (__p && ((__key = (typeof(__key))(long)__p->k) || 1)); \
       __p = __p->next)                                \

#define dll_is_tail(dll) ((dll)->prev == NULL)
#define dll_is_head(dll) ((dll)->next == NULL)

#define dll_append(dll, __k) __dll_append((dll), (void*)__k)
static node_t *__dll_append(node_t *dll, void* k) {
  node_t *n = Box((node_t){.k = (void*)k});
  n->prev = dll;
  if (dll_is_head(dll)) {
    dll->next = n;
  } else {
    dll->next->prev = n;
    dll->next = n;
  }
  return n;
}

#define dll_prepend(dll, __k) __dll_prepend((dll), (void*)__k)
static node_t *__dll_prepend(node_t *dll, void* k) {
  node_t *n = Box((node_t){.k = (void*)k});
  n->next = dll;
  if (dll_is_tail(dll)) {
    dll->prev =n;
  } else {
    dll->prev->next = n;
    dll->prev = n;
  }
  return n;
}

#define dll_foreach_next(dll, key) \
  for(node_t *__p = (dll), *__s = (dll), *__t = NULL; \
      __p && (!(__t++) || (__p != __s)) && \
        ((key = (typeof(key))(long)__p->k) || 1); \
      __p = __p->next)

#define dll_foreach_prev(dll, key) \
  for(node_t *__p = (dll), *__s = (dll), *__t = NULL; \
      __p && (!(__t++) || (__p != __s)) && \
        ((key = (typeof(key))(long)__p->k) || 1); \
      __p = __p->prev)

static node_t *dll_head(node_t *dll) {
  node_t *head = dll;
  while(head->next) head = head->next;
  return head;
}

static node_t *dll_tail(node_t *dll) {
  node_t *head = dll;
  while(head->prev) head = head->prev;
  return head;
}

/* End: Linked List */
/* Start: Hash Table */
#define MAGIC 5381
#define TABLE_SIZE 1024

struct _node {
const char* key;
int value;
struct _node* next;
};

typedef struct {
  struct _node* items[TABLE_SIZE];

} String2Int;

size_t __hash(const char *key) {
    size_t hash = MAGIC;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

#define ht_get(ht, __k, __v) do {                 \
    (__v) = NULL;                                 \
    size_t __i = __hash((__k)) % TABLE_SIZE;                   \
    typeof(**(ht)->items) *__c = (ht)->items[__i];             \
    while (__c != NULL) {                                      \
      if (strcmp(__c->key, (__k)) == 0) {                      \
        (__v) = __c;                                           \
        break;                                          \
      }                                                        \
      __c = __c->next;                                         \
    }                                                          \
} while(0);

#define ht_insert(ht, k, v) do {                                        \
    size_t __i = __hash((k)) % TABLE_SIZE;                              \
    typeof(**(ht)->items) *__n = malloc(sizeof(typeof(**(ht)->items))); \
    expect(__n != NULL);                                                \
    __n->key = strdup(k);                                               \
    __n->value = v;                                                     \
    __n->next = (ht)->items[__i];                                       \
    (ht)->items[__i] = __n;                                             \
  } while(0);

#define ht_contains(ht, k) ((ht)->items[__hash((k)) % TABLE_SIZE] != NULL)

/* End: Hash Table */

/* Start: Temporary strings */
#define format(fmt, ...) __format(fmt, __VA_ARGS__)

const char *__format(const char *fmt, ...) {
  expect(fmt != NULL);
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(__buf, __TMP_BUF_LEN, fmt, ap);

  return __buf;
}
/* End: Temporary strings */
#endif // _LIBPJ_H_
