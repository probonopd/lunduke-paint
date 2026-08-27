// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/application.hpp"

#include "app/actions.hpp"
#include "app/main_window.hpp"

#include <giomm/application.h>

namespace brushpad {

Glib::RefPtr<Application> Application::create() {
  return Glib::RefPtr<Application>(new Application());
}

Application::Application()
    : Gtk::Application(actions::kAppId, Gio::APPLICATION_FLAGS_NONE) {}

void Application::on_startup() {
  Gtk::Application::on_startup();

  add_action(actions::kNew, sigc::mem_fun(*this, &Application::on_action_new));
  add_action(actions::kOpen, sigc::mem_fun(*this, &Application::on_action_open));
  add_action(actions::kSave, sigc::mem_fun(*this, &Application::on_action_save));
  add_action(actions::kSaveAs, sigc::mem_fun(*this, &Application::on_action_save_as));
  add_action(actions::kQuit, sigc::mem_fun(*this, &Application::on_action_quit));

  set_accels_for_action("app.new", {"<Primary>n"});
  set_accels_for_action("app.open", {"<Primary>o"});
  set_accels_for_action("app.save", {"<Primary>s"});
  set_accels_for_action("app.save-as", {"<Primary><Shift>s"});
  set_accels_for_action("app.quit", {"<Primary>q"});
  set_accels_for_action("win.toggle-right-dock", {"F12"});
  set_accels_for_action("win.undo", {"<Primary>z"});
  set_accels_for_action("win.redo", {"<Primary>y", "<Primary><Shift>z"});
  set_accels_for_action("win.zoom-in", {"<Primary>plus", "<Primary>equal"});
  set_accels_for_action("win.zoom-out", {"<Primary>minus"});
  set_accels_for_action("win.zoom-100", {"<Primary>0"});
  set_accels_for_action("win.zoom-fit", {"<Primary>1"});
  set_accels_for_action("win.toggle-grid", {"<Primary>g"});
  set_accels_for_action("win.cut", {"<Primary>x"});
  set_accels_for_action("win.copy", {"<Primary>c"});
  set_accels_for_action("win.paste", {"<Primary>v"});
  set_accels_for_action("win.delete", {"Delete", "BackSpace"});
  set_accels_for_action("win.duplicate", {"<Primary>j"});
  set_accels_for_action("win.select-all", {"<Primary>a"});
  set_accels_for_action("win.deselect", {"<Primary>d"});
}

void Application::on_activate() {
  if (window_ == nullptr) {
    window_ = new MainWindow();
    add_window(*window_);
    window_->signal_hide().connect([this]() {
      MainWindow* dying = window_;
      window_ = nullptr;
      delete dying;
    });
  }
  window_->present();
}

void Application::on_action_new() {
  if (window_ != nullptr) {
    window_->action_new();
  }
}

void Application::on_action_open() {
  if (window_ != nullptr) {
    window_->action_open();
  }
}

void Application::on_action_save() {
  if (window_ != nullptr) {
    window_->action_save();
  }
}

void Application::on_action_save_as() {
  if (window_ != nullptr) {
    window_->action_save_as();
  }
}

void Application::on_action_quit() {
  if (window_ != nullptr && !window_->confirm_close()) {
    return;
  }
  quit();
}

}  // namespace brushpad
