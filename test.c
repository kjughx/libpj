#define __INIT_CAP 2
#define UNIT_TEST
#include "libpj.h"
#include <fcntl.h>
#include <unistd.h>

#define expect_op(a, o, b, fmt, ...)                                    \
  do {                                                                  \
    if (!((a)o(b))) {                                                   \
      printf(fmt "\n", __VA_ARGS__);                                    \
      abort();                                                          \
    }                                                                   \
  } while (0);

  #define expect_int_eq(a, b)                                         \
expect_op((int)(a), ==, (int)(b), "lhs != rhs: lhs = %d, rhs = %d", (int)(a), (int)(b))
#define expect_str_eq(a, b)                                             \
  do {                                                                  \
    if (!(a) || !(b) || strcmp((a), (b)) != 0) {                        \
      printf("lhs != rhs: lhs = `%s`, rhs = `%s`\n", (a), (b));         \
      abort();                                                          \
    }                                                                   \
  } while (0);

int double_it(int i) { return 2 * i; }

int main(void) {
  {  /* Dynamic Array */
    typedef struct {
      int *items;
      size_t count;
      size_t capacity;
    } Vec;

    Vec vec = {0};
    int i;
    da_append(&vec, 0);
    da_append(&vec, 1);
    expect(vec.count == 2);

    da_append(&vec, 2);
    expect(vec.count == 3);
    expectf(vec.capacity == __INIT_CAP * __GROWTH_RATE, "%zu != %zu",
            vec.capacity, (size_t)__INIT_CAP * __GROWTH_RATE);

    da_append(&vec, 3);
    da_foreach(&vec, i) { expect((int)__i == i); }

    da_map(&vec, double_it);
    da_foreach(&vec, i) { expect(2 * (int)__i == i); }
  }

  { /* Linear Algebra */
    typedef struct {
      int *items;
      size_t nx;
      size_t ny;
    } Matrix;

    Matrix ma = {.nx = 10, .ny = 10};
    ma_diag(&ma, 69);
    for (size_t x = 0; x < ma.nx; ++x) {
      for (size_t y = 0; y < ma.ny; ++y) {
        if (x == y) {
          expect(*ma_at(&ma, x, y) == 69);
        } else {
          expect(*ma_at(&ma, x, y) == 0);
        }
      }
    }
  }

  { /* String Builder */
    String_Builder sb = {0};
    sb_append(&sb, "Hello, ");
    sb_append(&sb, "World");
    sb_append(&sb, "!");
    sb_append(&sb, "\n");
    expect(strncmp(sb.items, "Hello, World!\n", sb.count) == 0);
    sb.count = 0;

    sb_appends(&sb, "Hello, ", "World", "!", "\n");
    expect(strncmp(sb.items, "Hello, World!\n", sb.count) == 0);

    sb.count = 0;
    sb_read_file(&sb, "./testfile");

    sb.count = 0;
    FILE *fp = fopen("./testfile", "r");
    sb_read_file(&sb, fp);
    fclose(fp);

    sb.count = 0;
    int fd = open("./testfile", O_RDONLY);
    sb_read_file(&sb, fd);
    close(fd);
  }

  { /* String View */
    String_Builder sb = {0};

    sb_read_file(&sb, "./testfile");
    String_View sv = sb_find(&sb, 'W');
    expect(sv.buf != NULL);
    expect(sv.size > 0);
    expect(*sv.buf == 'W');

    sv = sb_find(&sb, "World");
    expect(sv.buf != NULL);
    expect(sv.size > 0);
    expect(strncmp(sv_to_sb(sv).items, "World", 5) == 0);
  }

  { /* String Split */
    String_Builder sb = {0};

    sb_appends(&sb, "This is a sentence with many spaces");
    static char *splits[] = {"This", "is",   "a",     "sentence",
                             "with", "many", "spaces"};

    String_Split sp = sb_split(&sb, ' ');
    for (size_t i = 0; i < sp.count; ++i) {
      expect_str_eq(sv_to_cstr(sp.items[i]), splits[i]);
    }
  }

  { /* Format */
    expect_str_eq(format("%d %c %s %f", 42, 'd', "Hello, World!", 3.14),
                  "42 d Hello, World! 3.14000");
  }

  { /* Linked List */
    node_t ll = {.k = "hi"};
    ll_append(&ll, "bye");
    char *key;
    char *answers[2] = {"hi", "bye"};
    size_t i = 0;
    ll_foreach(&ll, key) {
      expect_str_eq(answers[i], key);
      i++;
    }

    node_t *dll = Box((node_t){ .k = (void*)1});
    int k;
    dll_prepend(dll_tail(dll), 2);
    dll_prepend(dll_tail(dll), 3);
    dll_prepend(dll_tail(dll), 4);
    i = 4;
    dll_foreach_next(dll_tail(dll), k) {
      expect_int_eq(i, k);
      i--;
    }
    i = 1;
    dll_foreach_prev(dll_head(dll), k) {
      expect_int_eq(i, k);
      i++;
    }
  }

  { /* Box */
    struct Struct {
      int x, y, z;
      char *s;
    };
    struct Struct s = {
      .x = 1,
      .y = 2,
      .z = 3,
      .s = "hi",
    };
    struct Struct *bs = Box(s);
    expect(memcmp(bs, &s, sizeof(*bs)) == 0);
  }
}
