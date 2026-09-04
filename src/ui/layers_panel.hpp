// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef LUNDUKEPAINT_UI_LAYERS_PANEL_HPP
#define LUNDUKEPAINT_UI_LAYERS_PANEL_HPP

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>

namespace lundukepaint {

class Document;

class LayersPanel : public Gtk::Box {
public:
  LayersPanel();

  void set_document(Document* document);
  void refresh();
  void show_properties();

private:
  void add_row(int stack_index);
  void on_row_selected(Gtk::ListBoxRow* row);
  void popup_row_menu(int stack_index, GdkEventButton* event);
  void rename_layer(int stack_index);
  int selected_stack_index() const;
  void update_buttons();

  Document* document_{nullptr};
  bool refreshing_{false};

  Gtk::ScrolledWindow scroll_;
  Gtk::ListBox list_;
  Gtk::ComboBoxText blend_;
  Gtk::Grid buttons_;
  Gtk::Button new_{"New"};
  Gtk::Button dup_{"Dup"};
  Gtk::Button del_{"Del"};
  Gtk::Button up_{"Up"};
  Gtk::Button down_{"Down"};
  Gtk::Button merge_{"Merge"};
  Gtk::Button flatten_{"Flat"};
};

}  // namespace lundukepaint

#endif
