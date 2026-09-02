// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/document.hpp"
#include "io/crash_recovery.hpp"

#include <cstdio>
#include <cstdlib>
#include <glib.h>
#include <string>
#include <unistd.h>

namespace {

using brushpad::Color;
using brushpad::Document;
using brushpad::crash_recovery::autosave_path;
using brushpad::crash_recovery::clear;
using brushpad::crash_recovery::exists;
using brushpad::crash_recovery::write_document;

int expect(bool cond, const char* msg) {
  if (!cond) {
    std::fprintf(stderr, "test_crash_recovery: %s\n", msg);
    return 1;
  }
  return 0;
}

}  // namespace

int main() {
  char dir[] = "/tmp/brushpad-state-XXXXXX";
  if (mkdtemp(dir) == nullptr) {
    std::fprintf(stderr, "test_crash_recovery: mkdtemp failed\n");
    return 1;
  }
  g_setenv("XDG_STATE_HOME", dir, TRUE);

  int errors = 0;
  errors += expect(!exists(), "no recovery file yet");

  auto doc = Document::create(4, 3, Color::white(), "Background");
  doc->layers().active_layer().set_pixel(1, 1, Color{255, 0, 0, 255});
  std::string error;
  errors += expect(write_document(*doc, error), "write recovery");
  if (!error.empty()) {
    std::fprintf(stderr, "test_crash_recovery: %s\n", error.c_str());
  }
  errors += expect(exists(), "recovery file exists");
  errors += expect(g_file_test(autosave_path().c_str(), G_FILE_TEST_IS_REGULAR) == TRUE,
                   "path is a regular file");

  clear();
  errors += expect(!exists(), "cleared");

  rmdir(dir);

  if (errors != 0) {
    std::fprintf(stderr, "test_crash_recovery: %d failure(s)\n", errors);
    return 1;
  }
  std::printf("test_crash_recovery: ok\n");
  return 0;
}
