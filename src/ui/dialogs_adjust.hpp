// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_DIALOGS_ADJUST_HPP
#define BRUSHPAD_UI_DIALOGS_ADJUST_HPP

#include <gtkmm/dialog.h>
#include <gtkmm/scale.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/window.h>

namespace brushpad {

class BrightnessContrastDialog : public Gtk::Dialog {
public:
  explicit BrightnessContrastDialog(Gtk::Window& parent);

  int brightness() const;
  int contrast() const;

private:
  Gtk::Scale brightness_;
  Gtk::Scale contrast_;
};

class HueSaturationDialog : public Gtk::Dialog {
public:
  explicit HueSaturationDialog(Gtk::Window& parent);

  int hue() const;
  int saturation() const;

private:
  Gtk::Scale hue_;
  Gtk::Scale saturation_;
};

class PosterizeDialog : public Gtk::Dialog {
public:
  explicit PosterizeDialog(Gtk::Window& parent);

  int levels() const;

private:
  Gtk::SpinButton levels_;
};

}  // namespace brushpad

#endif
