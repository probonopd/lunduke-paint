// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_LAYERS_PANEL_HPP
#define BRUSHPAD_UI_LAYERS_PANEL_HPP

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>

namespace brushpad {

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
  Gtk::Box blend_row_{Gtk::ORIENTATION_HORIZONTAL, 6};
  Gtk::Label blend_label_{"Blend"};
  Gtk::ComboBoxText blend_;
  Gtk::Box buttons_{Gtk::ORIENTATION_HORIZONTAL, 2};
  Gtk::Button new_{"New"};
  Gtk::Button dup_{"Dup"};
  Gtk::Button del_{"Del"};
  Gtk::Button up_{"Up"};
  Gtk::Button down_{"Down"};
  Gtk::Button merge_{"Merge"};
  Gtk::Button flatten_{"Flat"};
};

}  // namespace brushpad

#endif
