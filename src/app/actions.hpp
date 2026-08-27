// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_APP_ACTIONS_HPP
#define BRUSHPAD_APP_ACTIONS_HPP

namespace brushpad {
namespace actions {

constexpr const char* kAppId = "org.brushpad.Brushpad";

constexpr const char* kNew = "new";
constexpr const char* kOpen = "open";
constexpr const char* kSave = "save";
constexpr const char* kSaveAs = "save-as";
constexpr const char* kQuit = "quit";

constexpr const char* kUndo = "undo";
constexpr const char* kRedo = "redo";
constexpr const char* kToggleRightDock = "toggle-right-dock";
constexpr const char* kZoomIn = "zoom-in";
constexpr const char* kZoomOut = "zoom-out";
constexpr const char* kZoom100 = "zoom-100";
constexpr const char* kZoomFit = "zoom-fit";
constexpr const char* kToggleGrid = "toggle-grid";

constexpr const char* kCut = "cut";
constexpr const char* kCopy = "copy";
constexpr const char* kPaste = "paste";
constexpr const char* kDelete = "delete";
constexpr const char* kDuplicate = "duplicate";
constexpr const char* kSelectAll = "select-all";
constexpr const char* kDeselect = "deselect";
constexpr const char* kInvertSelection = "invert-selection";

constexpr const char* kCanvasSize = "canvas-size";
constexpr const char* kScale = "scale";
constexpr const char* kCrop = "crop";
constexpr const char* kAutocrop = "autocrop";
constexpr const char* kRotate90 = "rotate-90";
constexpr const char* kRotate180 = "rotate-180";
constexpr const char* kFlipH = "flip-h";
constexpr const char* kFlipV = "flip-v";
constexpr const char* kClear = "clear";

}  // namespace actions
}  // namespace brushpad

#endif
