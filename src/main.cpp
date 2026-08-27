// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/application.hpp"

#include <glib.h>

int main(int argc, char* argv[]) {
  if (g_getenv("GDK_BACKEND") == nullptr) {
    g_setenv("GDK_BACKEND", "x11", FALSE);
  }

  auto app = brushpad::Application::create();
  return app->run(argc, argv);
}
