// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_APP_APPLICATION_HPP
#define BRUSHPAD_APP_APPLICATION_HPP

#include <glibmm/refptr.h>
#include <gtkmm/application.h>

namespace brushpad {

class MainWindow;

class Application : public Gtk::Application {
public:
  static Glib::RefPtr<Application> create();

  MainWindow* main_window() const { return window_; }

protected:
  Application();

  void on_startup() override;
  void on_activate() override;

private:
  void on_action_new();
  void on_action_open();
  void on_action_save();
  void on_action_quit();

  MainWindow* window_{nullptr};
};

}  // namespace brushpad

#endif
