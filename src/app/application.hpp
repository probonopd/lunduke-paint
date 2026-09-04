// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef LUNDUKEPAINT_APP_APPLICATION_HPP
#define LUNDUKEPAINT_APP_APPLICATION_HPP

#include <giomm/file.h>
#include <glibmm/refptr.h>
#include <gtkmm/application.h>

namespace lundukepaint {

class MainWindow;

class Application : public Gtk::Application {
public:
  static Glib::RefPtr<Application> create();

  MainWindow* main_window() const { return window_; }

protected:
  Application();

  void on_startup() override;
  void on_activate() override;
  void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;

private:
  // Returns true when this call actually created the window (a genuinely fresh
  // launch), false when an existing window was reused.
  bool ensure_window();
  void on_action_new();
  void on_action_open();
  void on_action_save();
  void on_action_save_as();
  void on_action_quit();

  MainWindow* window_{nullptr};
};

}  // namespace lundukepaint

#endif
