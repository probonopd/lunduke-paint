// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdio>

int main() {
  if ((1 + 1) != 2) {
    std::fprintf(stderr, "dummy: expected 1+1 == 2\n");
    return 1;
  }
  std::printf("dummy: ok\n");
  return 0;
}
