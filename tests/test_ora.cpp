// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/document.hpp"
#include "io/ora.hpp"
#include "raster/blend.hpp"

#include <cstdio>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

using brushpad::BlendMode;
using brushpad::Color;
using brushpad::Document;
using brushpad::Layer;
using brushpad::LoadedOra;
using brushpad::load_ora;
using brushpad::save_ora;

int expect(bool cond, const char* msg) {
  if (!cond) {
    std::fprintf(stderr, "test_ora: %s\n", msg);
    return 1;
  }
  return 0;
}

std::string temp_ora_path() {
  char path[] = "/tmp/brushpad-ora-XXXXXX";
  const int fd = mkstemp(path);
  if (fd >= 0) {
    close(fd);
    unlink(path);
  }
  return std::string(path) + ".ora";
}

}  // namespace

int main() {
  int errors = 0;
  auto doc = Document::create(8, 6, Color::white(), "Background");
  doc->add_layer();
  Layer& top = doc->layers().active_layer();
  top.set_name("Overlay");
  top.set_opacity(0.5f);
  top.set_visible(false);
  top.set_blend(BlendMode::Multiply);
  top.set_pixel(1, 1, Color{255, 0, 0, 255});
  doc->layers().at(0).set_pixel(0, 0, Color{0, 0, 255, 255});

  const std::string path = temp_ora_path();
  std::string error;
  errors += expect(save_ora(path, *doc, error), "save_ora succeeded");
  if (!error.empty()) {
    std::fprintf(stderr, "test_ora: save error: %s\n", error.c_str());
  }

  LoadedOra loaded = load_ora(path);
  errors += expect(loaded.ok(), "load_ora succeeded");
  if (!loaded.ok()) {
    std::fprintf(stderr, "test_ora: load error: %s\n", loaded.error.c_str());
  }
  errors += expect(loaded.width == 8 && loaded.height == 6, "size");
  errors += expect(static_cast<int>(loaded.layers.size()) == 2, "two layers");
  if (loaded.layers.size() == 2) {
    errors += expect(loaded.layers[0].name == "Background", "bottom name");
    errors += expect(loaded.layers[1].name == "Overlay", "top name");
    errors += expect(loaded.layers[1].opacity > 0.49f && loaded.layers[1].opacity < 0.51f,
                     "top opacity");
    errors += expect(loaded.layers[1].visible == false, "top hidden");
    errors += expect(loaded.layers[1].blend == BlendMode::Multiply, "top blend");
    const auto& bg = loaded.layers[0].pixels;
    errors += expect(!bg.empty() && bg[0] == 0 && bg[1] == 0 && bg[2] == 255 && bg[3] == 255,
                     "bottom pixel");
    const auto& ov = loaded.layers[1].pixels;
    const int idx = (1 * 8 + 1) * 4;
    errors += expect(static_cast<int>(ov.size()) > idx + 3 && ov[idx] == 255 && ov[idx + 1] == 0 &&
                         ov[idx + 2] == 0,
                     "top pixel");
  }

  unlink(path.c_str());

  if (errors != 0) {
    std::fprintf(stderr, "test_ora: %d failure(s)\n", errors);
    return 1;
  }
  std::printf("test_ora: ok\n");
  return 0;
}
