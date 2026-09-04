// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef LUNDUKEPAINT_IO_CRASH_RECOVERY_HPP
#define LUNDUKEPAINT_IO_CRASH_RECOVERY_HPP

#include <string>

namespace lundukepaint {

class Document;

namespace crash_recovery {

std::string state_dir();
std::string autosave_path();
bool exists();
bool write_document(const Document& document, std::string& error);
void clear();

}  // namespace crash_recovery

}  // namespace lundukepaint

#endif
