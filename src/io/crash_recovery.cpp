// SPDX-License-Identifier: GPL-3.0-or-later

#include "io/crash_recovery.hpp"

#include "io/ora.hpp"

#include <glib.h>

#include <string>
#include <unistd.h>

namespace brushpad {
namespace crash_recovery {

std::string state_dir() {
  const char* env = g_getenv("XDG_STATE_HOME");
  std::string base;
  if (env != nullptr && env[0] != '\0') {
    base = env;
  } else {
    const char* home = g_get_home_dir();
    if (home == nullptr || home[0] == '\0') {
      home = ".";
    }
    base = std::string(home) + "/.local/state";
  }
  return base + "/lunduke-paint";
}

std::string autosave_path() {
  return state_dir() + "/recovery.ora";
}

bool exists() {
  return g_file_test(autosave_path().c_str(), G_FILE_TEST_IS_REGULAR) == TRUE;
}

bool write_document(const Document& document, std::string& error) {
  const std::string dir = state_dir();
  g_mkdir_with_parents(dir.c_str(), 0755);
  const std::string dest = autosave_path();
  const std::string tmp = dest + ".tmp";
  if (!save_ora(tmp, document, error)) {
    unlink(tmp.c_str());
    return false;
  }
  if (rename(tmp.c_str(), dest.c_str()) != 0) {
    error = "Could not replace crash-recovery file";
    unlink(tmp.c_str());
    return false;
  }
  return true;
}

void clear() {
  unlink(autosave_path().c_str());
}

}  // namespace crash_recovery
}  // namespace brushpad
