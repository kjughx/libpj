#define __INIT_CAP 2
#define UNIT_TEST
#include "libpj.h"
#include <fcntl.h>
#include <unistd.h>


typedef struct {
  int *items;
  size_t count;
  size_t capacity;
} Vec;

int main(void) {
  Vec vec = {0};
  int i;
  da_append(&vec, 0);
  da_append(&vec, 1);
  expect(vec.count == 2);

  da_append(&vec, 2);
  expect(vec.count == 3);
  expectf(vec.capacity == __INIT_CAP * __GROWTH_RATE,
          "%zu != %zu", vec.capacity, (size_t)__INIT_CAP * __GROWTH_RATE);

  da_append(&vec, 3);
  da_foreach(&vec, i) {
    expect((int)__i == i);
  }

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
