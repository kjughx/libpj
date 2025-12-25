#ifndef _LIBPJ_H_
#define _LIBPJ_H_

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Temporary buffer */
#define __TMP_BUF_LEN 1024
static char __buf[__TMP_BUF_LEN] = {0};

/* Start: Useful macros */
#define expect(cond)                                                           \
  do {                                                                         \
    int c = (cond);                                                            \
    if (!c) {                                                                  \
      printf("%s:%d: Expected `%s`, got %d\n", __FILE__, __LINE__, #cond, c);  \
    }                                                                          \
  } while (0);
#define expectf(cond, ...) _expectf(cond, __VA_ARGS__)
#define _expectf(cond, fmt, ...)                                               \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("%s:%d: Expected `%s`, " fmt "\n", __FILE__, __LINE__, #cond,     \
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

#define swap(x, y)                                                             \
  do {                                                                         \
    _Static_assert(sizeof(x) == sizeof(y));                                    \
    typeof(x) __t = (y);                                                       \
    (y) = (x);                                                                 \
    (x) = __t;                                                                 \
  } while (0);

static inline void __print_int(int x) { printf("%d\n", x); }
static inline void __print_str(char *x) { printf("%s\n", x); }
#define print(x) _Generic((x), int: __print_int, char *: __print_str)(x);
#define println(fmt, ...) (printf(fmt "\n", __VA_ARGS__))

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define SETBIT(x, n) ((x) |= (1llu << (n)))
#define CLRBIT(x, n) ((x) &= (~(1llu << (n))))
#define IS_SET(x, n) ((x) & (1llu << (n)))

#define loop(i) for (size_t __i = 0; __i < (i); ++__i)

/* End: Useful macros */

/* Start: Types */

typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;

typedef struct {
  char *items;
  size_t nx;
  size_t ny;
} Grid;

/* End: Types */

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
    (da)->items[(da)->count] = (x);                                            \
    (da)->count++;                                                             \
  } while (0);

#define da_map(da, f)                                                          \
  do {                                                                         \
    __item_type((da)) * __item;                                                \
    da_foreach_ref((da), __item) { *__item = f(*__item); }                     \
  } while (0);

#define da_sort(da, f) (qsort((da)->items, (da)->count, __item_size((da)), (f)))

#define da_pop(da) ((da)->items[--(da)->count])

/* End: DYNAMIC ARRAY */

/* Start: Box */
#define Box(x)                                                                 \
  _Generic((x), char *: __box_str, default: __box)(&x, sizeof((x)));
static inline void *__box(void *x, size_t s) {
  void *p = malloc(s);
  memcpy(p, x, s);
  return p;
}

static inline char *__box_str(void *x, size_t s) {
  UNUSED(s);
  char *str = *(char **)x;
  return strdup(str);
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
    expect((ma)->items != NULL);                                       \
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

#define ma_zero(ma) ma_fill((ma), 0)

#define ma_inbounds(ma, x, y)                                                  \
  ((0 <= (x) && (x) < (typeof((x)))(ma)->nx) &&                                \
   (0 <= (y) && (y) < (typeof((y)))(ma)->ny))

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
  ssize_t x, y, z;
} Vector3;

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

#define sb_skip_word(sb)                                                       \
  do {                                                                         \
    while (*(sb)->items && isalnum(*(sb)->items)) {                            \
      (sb)->items++;                                                           \
      (sb)->count--;                                                           \
    }                                                                          \
    while (*(sb)->items && isspace(*(sb)->items)) {                            \
      (sb)->items++;                                                           \
      (sb)->count--;                                                           \
    }                                                                          \
  } while (0);

static inline char *sb_get_words(String_Builder *sb, int n) {
  char *tmp = sb->items;
  while (n--) {
    sb_skip_word(sb);
  }
  return strndup(tmp, sb->items - tmp - 1);
}

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

#define sb_from_cstr(__cstr)                                                   \
  (String_Builder) {                                                           \
    .items = strdup(__cstr), .count = strlen(__cstr),                          \
    .capacity = strlen(__cstr)                                                 \
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
  if (sb->count == 0)
    return;
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
      .size = tmp.count,
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

#define sb_foreach_line(sb, __l)                                               \
  for ((__l) = strtok((sb)->items, "\n"); (__l); __l = strtok(NULL, "\n"))

/* End: STRING BUILDER */

/* Start: Linked List */
typedef struct node {
  void *k;
  void *v;
  struct node *next;
  struct node *prev;
} node_t;

#define ll_append(ll, __k)                                                     \
  do {                                                                         \
    (ll)->next = Box((node_t){.k = (void *)__k});                              \
  } while (0);

#define ll_foreach(ll, __key)                                                  \
  for (node_t *__p = (ll);                                                     \
       (__p && ((__key = (typeof(__key))(long)__p->k) || 1)); __p = __p->next)

#define dll_is_tail(dll) ((dll)->prev == NULL)
#define dll_is_head(dll) ((dll)->next == NULL)

#define dll_append(dll, __k) __dll_append((dll), (void *)__k)
static node_t *__dll_append(node_t *dll, void *k) {
  node_t _n = {.k = (void *)k};
  node_t *n = Box(_n);
  n->prev = dll;
  if (dll_is_head(dll)) {
    dll->next = n;
  } else {
    dll->next->prev = n;
    dll->next = n;
  }
  return n;
}

#define dll_prepend(dll, __k) __dll_prepend((dll), (void *)__k)
static node_t *__dll_prepend(node_t *dll, void *k) {
  node_t _n = {.k = (void *)k};
  node_t *n = Box(_n);
  n->next = dll;
  if (dll_is_tail(dll)) {
    dll->prev = n;
  } else {
    dll->prev->next = n;
    dll->prev = n;
  }
  return n;
}

#define dll_foreach_next(dll, key)                                             \
  for (node_t *__p = (dll), *__s = (dll), *__t = NULL;                         \
       __p && (!(__t++) || (__p != __s)) &&                                    \
       ((key = (typeof(key))(long)__p->k) || 1);                               \
       __p = __p->next)

#define dll_foreach_prev(dll, key)                                             \
  for (node_t *__p = (dll), *__s = (dll), *__t = NULL;                         \
       __p && (!(__t++) || (__p != __s)) &&                                    \
       ((key = (typeof(key))(long)__p->k) || 1);                               \
       __p = __p->prev)

static node_t *dll_head(node_t *dll) {
  node_t *head = dll;
  while (head->next)
    head = head->next;
  return head;
}

static node_t *dll_tail(node_t *dll) {
  node_t *head = dll;
  while (head->prev)
    head = head->prev;
  return head;
}

/* End: Linked List */
/* Start: Hash Table */
#define MAGIC 5381
#define TABLE_SIZE 50000

struct __node_String2Int {
  char *key;
  u64 *value;
  struct __node_String2Int *next;
};

typedef struct {
  struct __node_String2Int *nodes[TABLE_SIZE];

  char **items;
  size_t count;
  size_t capacity;
} String2Int;

struct __node_Int2Int {
  u64 *key;
  u64 *value;
  struct __node_Int2Int *next;
};

typedef struct {
  struct __node_Int2Int *nodes[TABLE_SIZE];

  u64 **items;
  size_t count;
  size_t capacity;

} Int2Int;

struct __node_Vector22Int {
  Vector2 *key;
  u64 *value;
  struct __node_Vector22Int *next;
};

typedef struct {
  struct __node_Vector22Int *nodes[TABLE_SIZE];

  Vector2 **items;
  size_t count;
  size_t capacity;
} Vector22Int;

struct __node_Vector32Int {
  Vector3 *key;
  u64 *value;
  struct __node_Vector32Int *next;
};

typedef struct {
  struct __node_Vector32Int *nodes[TABLE_SIZE];

  Vector3 **items;
  size_t count;
  size_t capacity;
} Vector32Int;

struct __node_DASet {
  u64 *key;
  struct __node_DASet *next;
};

typedef struct {
  struct __node_DASet *nodes[TABLE_SIZE];

  void ***items;
  size_t count;
  size_t capacity;
} DASet;

#define HT_DECL(name, keytype, valtype)                                        \
  struct __node##name {                                                        \
    char *key;                                                                 \
    valtype *value;                                                            \
    struct __node##name *next;                                                 \
  };                                                                           \
  typedef struct {                                                             \
    struct __node##name *nodes[TABLE_SIZE];                                    \
    char **items;                                                              \
    size_t count;                                                              \
    size_t capacity;                                                           \
  } name;

size_t __hash_str(const char *key) {
  size_t hash = MAGIC;
  int c;
  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  }
  return hash;
}

size_t __hash_u64(u64 key) {
  size_t hash = MAGIC;
  int c;
  while ((c = (key & 0xff))) {
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
    key >>= 8;
  }
  return hash;
}
size_t __hash_v2(Vector2 key) {
  return __hash_u64(key.x) + __hash_u64(key.y);
}
size_t __hash_v3(Vector3 key) {
  return __hash_u64(key.x) + __hash_u64(key.y) + __hash_u64(key.z);
}

#define __hash(__k) _Generic((__k), char *: __hash_str, u64: __hash_u64, Vector2: __hash_v2, Vector3: __hash_v3)((__k))

#define ht_get(ht, __k) _Generic((__k),                                 \
                                 char *: __ht_get_str,                  \
                                 Vector2: __ht_get_v2,                  \
                                 Vector3: __ht_get_v3,                  \
                                 default: __ht_get)((ht), (__k), offsetof(typeof(*(ht)->nodes[0]), key), \
                                                    offsetof(typeof(*(ht)), nodes), sizeof(*(ht)->nodes), \
                                                    offsetof(typeof(*(ht)->nodes[0]), value), \
                                                    offsetof(typeof(*(ht)->nodes[0]), next))

static inline void *__ht_get_str(void *ht, char* key, size_t key_offset,
                             size_t nodes_offset, size_t node_size,
                             size_t value_offset, size_t next_offset) {
  size_t nodes = (size_t)ht + nodes_offset; // &nodes[0]
  size_t idx = __hash(key) % TABLE_SIZE;
  void *node = *(void **)((size_t)nodes + idx * node_size); // nodes[idx] -> struct node*
  if (node == NULL) return NULL;

  char *_key = NULL;

  /* Now start searching the linked list, for a node with the same key */
  while (node != NULL) {
    _key = *(char **)((size_t)node + key_offset);
    if (strcmp(key, _key) == 0) break;
    node = *(void **)(((size_t)node) + next_offset); // node->next
  }

  if (node == NULL) return NULL;

  void *p = *(void**)(((size_t)node) + value_offset); // &node->value
  return p;
}

static inline void *__ht_get(void *ht, u64 key, size_t key_offset,
                             size_t nodes_offset, size_t node_size,
                             size_t value_offset, size_t next_offset) {
  size_t nodes = (size_t)ht + nodes_offset; // &nodes[0]
  size_t idx = __hash(key) % TABLE_SIZE;
  void *node = *(void **)((size_t)nodes + idx * node_size); // nodes[idx] -> struct node*
  if (node == NULL) return NULL;

  u64 *_key = NULL;

  /* Now start searching the linked list, for a node with the same key */
  while (node != NULL) {
    _key = *(u64 **)((size_t)node + key_offset);
    if (key == *_key) break;
    node = *(void **)(((size_t)node) + next_offset); // node->next
  }

  if (node == NULL) return NULL;

  void *p = *(void**)(((size_t)node) + value_offset); // &node->value
  return p;
}

static inline void *__ht_get_v2(void *ht, Vector2 key, size_t key_offset,
                             size_t nodes_offset, size_t node_size,
                             size_t value_offset, size_t next_offset) {
  size_t nodes = (size_t)ht + nodes_offset; // &nodes[0]
  size_t idx = __hash(key) % TABLE_SIZE;
  void *node = *(void **)((size_t)nodes + idx * node_size); // nodes[idx] -> struct node*
  if (node == NULL) return NULL;

  Vector2 *_key = NULL;

  /* Now start searching the linked list, for a node with the same key */
  while (node != NULL) {
    _key = *(Vector2 **)((size_t)node + key_offset);
    if (memcmp(&key, _key, sizeof(key)) == 0) break;
    node = *(void **)(((size_t)node) + next_offset); // node->next
  }

  if (node == NULL) return NULL;

  void *p = *(void**)(((size_t)node) + value_offset); // &node->value
  return p;
}

static inline void *__ht_get_v3(void *ht, Vector3 key, size_t key_offset,
                             size_t nodes_offset, size_t node_size,
                             size_t value_offset, size_t next_offset) {
  size_t nodes = (size_t)ht + nodes_offset; // &nodes[0]
  size_t idx = __hash(key) % TABLE_SIZE;
  void *node = *(void **)((size_t)nodes + idx * node_size); // nodes[idx] -> struct node*
  if (node == NULL) return NULL;

  Vector3 *_key = NULL;

  /* Now start searching the linked list, for a node with the same key */
  while (node != NULL) {
    _key = *(Vector3 **)((size_t)node + key_offset);
    if (memcmp(&key, _key, sizeof(key)) == 0) break;
    node = *(void **)(((size_t)node) + next_offset); // node->next
  }

  if (node == NULL) return NULL;

  void *p = *(void**)(((size_t)node) + value_offset); // &node->value
  return p;
}

#define ht_insert(ht, k, v)                                                    \
  do {                                                                         \
    size_t __i = __hash((k)) % TABLE_SIZE;                                     \
    typeof(v) __v = v;                                                         \
    typeof(k) __k = k;                                                  \
    typeof(**(ht)->nodes) *__n = malloc(sizeof(typeof(**(ht)->nodes)));        \
    expect(__n != NULL);                                                       \
    __n->key = Box(__k);                                                \
    __n->value = Box(__v);                                                     \
    __n->next = (ht)->nodes[__i];                                              \
    (ht)->nodes[__i] = __n;                                                    \
    da_append((ht), __n->key);                                                 \
  } while (0);

#define ht_contains(ht, k) ((ht)->items[__hash((k)) % TABLE_SIZE] != NULL)

#define ht_clear(ht) do { \
  for (size_t __i = 0; __i < (ht)->count; ++__i) { \
    size_t __idx = __hash(*(ht)->items[__i]) % TABLE_SIZE; \
    if ((ht)->nodes[__idx]) { \
      void *n = (void*)(ht)->nodes[__i];        \
      while (n) {                               \
        void *next = *(void**)(((size_t)n) + offsetof(typeof(*(ht)->nodes[0]), next)); \
        free(n);                                                        \
        n = next;                                                       \
      }                                         \
    } \
    (ht)->nodes[__idx] = NULL;                  \
    (ht)->count = 0;                            \
  } \
} while(0);

/* End: Hash Table */

/* Start: Temporary strings */
#define format(fmt, ...) __format(fmt, __VA_ARGS__)

char *__format(const char *fmt, ...) {
  expect(fmt != NULL);
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(__buf, __TMP_BUF_LEN, fmt, ap);

  return __buf;
}
/* End: Temporary strings */

/* Start: Math */
/* End: Math */

/* Start: Grid */
#define grid_read(p) _Generic((p), FILE*: __grid_read_fp(p))
static inline Grid __grid_read_fp(FILE *p) {
  String_Builder sb = {0};
  sb_read_file(&sb, p);
  sb_strip(&sb, '\n');
  String_Split lines = sb_split(&sb, '\n');
  Grid G = {.nx = lines.items[0].size, .ny = lines.count};
  ma_init(&G);

  for (size_t y = 0; y < G.ny; ++y) {
    for (size_t x = 0; x < G.nx; ++x) {
      *ma_at(&G, x, y) = lines.items[y].buf[x];
    }
  }
  return G;
}

static inline void grid_print(Grid *G) {
  for (size_t y = 0; y < G->ny; ++y) {
    for (size_t x = 0; x < G->nx; ++x) {
      printf("%c", *ma_at(G, x, y));
    }
    printf("\n");
  }
}
/* End: Grid */
#endif // _LIBPJ_H_
