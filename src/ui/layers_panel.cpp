// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/layers_panel.hpp"

#include "doc/document.hpp"
#include "raster/blend.hpp"

#include <gdkmm/pixbuf.h>
#include <gtkmm/entry.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/window.h>

#include <cstring>
#include <functional>
#include <memory>
#include <vector>
#include <gtk/gtk.h>
#include <gtkmm/cellrenderertext.h>

namespace brushpad {
namespace {

Glib::RefPtr<Gdk::Pixbuf> thumb_pixbuf(const Layer& layer) {
  const int w = layer.thumbnail_width();
  const int h = layer.thumbnail_height();
  auto pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, w, h);
  const int dst_stride = pixbuf->get_rowstride();
  std::uint8_t* dst = pixbuf->get_pixels();
  const std::uint8_t* src = layer.thumbnail();
  for (int y = 0; y < h; ++y) {
    std::memcpy(dst + static_cast<std::size_t>(y) * dst_stride,
                src + static_cast<std::size_t>(y) * static_cast<std::size_t>(w) * 4,
                static_cast<std::size_t>(w) * 4);
  }
  return pixbuf;
}

Gtk::Window* toplevel_window(Gtk::Widget& widget) {
  auto* top = widget.get_toplevel();
  if (top != nullptr && top->get_is_toplevel()) {
    return dynamic_cast<Gtk::Window*>(top);
  }
  return nullptr;
}

class LayerRow : public Gtk::ListBoxRow {
public:
  explicit LayerRow(int stack_index) : stack_index(stack_index) {}
  int stack_index = 0;
};

}  // namespace

LayersPanel::LayersPanel() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2) {
  set_border_width(3);
  list_.set_selection_mode(Gtk::SELECTION_SINGLE);
  list_.set_activate_on_single_click(true);
  list_.signal_row_selected().connect(sigc::mem_fun(*this, &LayersPanel::on_row_selected));
  scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  scroll_.set_min_content_height(120);
  scroll_.add(list_);

  for (int i = 0; i < kBlendModeCount; ++i) {
    const BlendMode mode = blend_mode_from_index(i);
    blend_.append(blend_mode_label(mode));
  }
  blend_.set_active(0);
  blend_.set_tooltip_text("Blend");
  blend_.set_size_request(76, -1);
  blend_.set_hexpand(false);
  {
    auto* cell = blend_.get_first_cell();
    if (auto* text = dynamic_cast<Gtk::CellRendererText*>(cell)) {
      text->property_ellipsize() = Pango::ELLIPSIZE_END;
    }
  }
  blend_.signal_changed().connect([this]() {
    if (refreshing_ || document_ == nullptr) {
      return;
    }
    document_->set_layer_blend(document_->layers().active_index(),
                               blend_mode_from_index(blend_.get_active_row_number()));
  });

  buttons_.set_row_spacing(1);
  buttons_.set_column_spacing(1);
  buttons_.set_hexpand(true);
  auto pack_btn = [this](Gtk::Button& b, const char* icon, const char* tip, int col, int row) {
    b.set_label("");
    b.set_image_from_icon_name(icon, Gtk::ICON_SIZE_MENU);
    b.set_tooltip_text(tip);
    b.set_can_focus(false);
    b.set_relief(Gtk::RELIEF_NONE);
    b.set_hexpand(true);
    buttons_.attach(b, col, row, 1, 1);
  };
  pack_btn(new_, "list-add-symbolic", "New layer", 0, 0);
  pack_btn(dup_, "edit-copy-symbolic", "Duplicate layer", 1, 0);
  pack_btn(del_, "edit-delete-symbolic", "Delete layer", 0, 1);
  pack_btn(up_, "go-up-symbolic", "Raise layer", 1, 1);
  pack_btn(down_, "go-down-symbolic", "Lower layer", 0, 2);
  pack_btn(merge_, "go-bottom-symbolic", "Merge down", 1, 2);
  pack_btn(flatten_, "view-restore-symbolic", "Flatten", 0, 3);

  new_.signal_clicked().connect([this]() {
    if (document_ == nullptr) {
      return;
    }
    if (document_->layers().count() >= kSoftMaxLayers) {
      if (auto* win = toplevel_window(*this)) {
        Gtk::MessageDialog warn(*win,
                                "This document has 64 or more layers and may use a lot of memory.",
                                false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
        if (warn.run() != Gtk::RESPONSE_OK) {
          return;
        }
      }
    }
    document_->add_layer();
  });
  dup_.signal_clicked().connect([this]() {
    if (document_ != nullptr) {
      document_->duplicate_layer();
    }
  });
  del_.signal_clicked().connect([this]() {
    if (document_ != nullptr) {
      document_->delete_layer();
    }
  });
  up_.signal_clicked().connect([this]() {
    if (document_ != nullptr) {
      document_->raise_layer();
    }
  });
  down_.signal_clicked().connect([this]() {
    if (document_ != nullptr) {
      document_->lower_layer();
    }
  });
  merge_.signal_clicked().connect([this]() {
    if (document_ != nullptr) {
      document_->merge_down();
    }
  });
  flatten_.signal_clicked().connect([this]() {
    if (document_ != nullptr) {
      document_->flatten();
    }
  });

  pack_start(scroll_, Gtk::PACK_EXPAND_WIDGET);
  pack_start(blend_, Gtk::PACK_SHRINK);
  pack_start(buttons_, Gtk::PACK_SHRINK);
}

void LayersPanel::set_document(Document* document) {
  document_ = document;
  refresh();
}

int LayersPanel::selected_stack_index() const {
  auto* row = list_.get_selected_row();
  if (row == nullptr) {
    return document_ != nullptr ? document_->layers().active_index() : 0;
  }
  if (auto* layer_row = dynamic_cast<const LayerRow*>(row)) {
    return layer_row->stack_index;
  }
  return 0;
}

void LayersPanel::on_row_selected(Gtk::ListBoxRow* row) {
  if (refreshing_ || document_ == nullptr || row == nullptr) {
    return;
  }
  if (auto* layer_row = dynamic_cast<const LayerRow*>(row)) {
    document_->layers().set_active_index(layer_row->stack_index);
    document_->notify_changed();
  }
}

void LayersPanel::add_row(int stack_index) {
  if (document_ == nullptr) {
    return;
  }
  Layer& layer = document_->layers().at(stack_index);
  auto* row = Gtk::manage(new LayerRow(stack_index));
  auto* box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 1));
  box->set_border_width(1);

  auto* image = Gtk::manage(new Gtk::Image());
  auto full = thumb_pixbuf(layer);
  image->set(full->scale_simple(24, 18, Gdk::INTERP_NEAREST));
  image->set_halign(Gtk::ALIGN_START);

  auto* name = Gtk::manage(new Gtk::Label(layer.name()));
  name->set_xalign(0.0f);
  name->set_ellipsize(Pango::ELLIPSIZE_END);
  name->set_max_width_chars(8);

  auto* eye = Gtk::manage(new Gtk::ToggleButton());
  eye->set_image_from_icon_name("view-reveal-symbolic", Gtk::ICON_SIZE_MENU);
  eye->set_tooltip_text("Visible");
  eye->set_active(layer.visible());
  eye->set_can_focus(false);
  eye->signal_toggled().connect([this, stack_index, eye]() {
    if (refreshing_ || document_ == nullptr) {
      return;
    }
    document_->set_layer_visible(stack_index, eye->get_active());
  });

  auto* lock = Gtk::manage(new Gtk::ToggleButton());
  lock->set_image_from_icon_name("changes-prevent-symbolic", Gtk::ICON_SIZE_MENU);
  lock->set_tooltip_text("Locked");
  lock->set_active(layer.locked());
  lock->set_can_focus(false);
  lock->signal_toggled().connect([this, stack_index, lock]() {
    if (refreshing_ || document_ == nullptr) {
      return;
    }
    document_->set_layer_locked(stack_index, lock->get_active());
  });

  auto* toggles = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 1));
  toggles->pack_start(*eye, Gtk::PACK_SHRINK);
  toggles->pack_start(*lock, Gtk::PACK_SHRINK);

  box->pack_start(*image, Gtk::PACK_SHRINK);
  box->pack_start(*name, Gtk::PACK_SHRINK);
  box->pack_start(*toggles, Gtk::PACK_SHRINK);
  row->add(*box);
  row->add_events(Gdk::BUTTON_PRESS_MASK);
  row->signal_button_press_event().connect(
      [this, stack_index](GdkEventButton* event) {
        if (event != nullptr && event->button == 3) {
          popup_row_menu(stack_index, event);
          return true;
        }
        return false;
      },
      false);
  row->show_all();
  list_.append(*row);
}

void LayersPanel::popup_row_menu(int stack_index, GdkEventButton* event) {
  if (document_ == nullptr) {
    return;
  }
  document_->layers().set_active_index(stack_index);
  document_->notify_changed();

  auto menu = std::make_shared<Gtk::Menu>();
  auto add_item = [&](const char* label, std::function<void()> fn) {
    auto* item = Gtk::manage(new Gtk::MenuItem(label, true));
    item->signal_activate().connect(std::move(fn));
    menu->append(*item);
  };
  add_item("_New", [this]() { new_.clicked(); });
  add_item("_Duplicate", [this]() { document_->duplicate_layer(); });
  add_item("De_lete", [this]() { document_->delete_layer(); });
  add_item("_Raise", [this]() { document_->raise_layer(); });
  add_item("_Lower", [this]() { document_->lower_layer(); });
  add_item("_Merge down", [this]() { document_->merge_down(); });
  add_item("_Flatten", [this]() { document_->flatten(); });
  menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
  add_item("Re_name…", [this, stack_index]() { rename_layer(stack_index); });
  add_item("_Properties…", [this]() { show_properties(); });
  menu->show_all();
  menu->attach_to_widget(*this);
  if (event != nullptr) {
    menu->popup(event->button, event->time);
  } else {
    menu->popup(0, gtk_get_current_event_time());
  }
  // Keep the menu alive until it is popped down.
  menu->signal_hide().connect([menu]() mutable { menu.reset(); });
}

void LayersPanel::rename_layer(int stack_index) {
  if (document_ == nullptr) {
    return;
  }
  auto* win = toplevel_window(*this);
  Gtk::Dialog dialog("Rename layer", false);
  if (win != nullptr) {
    dialog.set_transient_for(*win);
  }
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Rename", Gtk::RESPONSE_OK);
  dialog.set_default_response(Gtk::RESPONSE_OK);
  Gtk::Entry entry;
  entry.set_text(document_->layers().at(stack_index).name());
  entry.set_activates_default(true);
  dialog.get_content_area()->set_border_width(8);
  dialog.get_content_area()->pack_start(entry, Gtk::PACK_SHRINK);
  dialog.show_all();
  if (dialog.run() == Gtk::RESPONSE_OK) {
    document_->rename_layer(stack_index, entry.get_text());
  }
}

void LayersPanel::show_properties() {
  if (document_ == nullptr) {
    return;
  }
  const int index = document_->layers().active_index();
  Layer& layer = document_->layers().at(index);
  auto* win = toplevel_window(*this);
  Gtk::Dialog dialog("Layer properties", false);
  if (win != nullptr) {
    dialog.set_transient_for(*win);
  }
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_OK", Gtk::RESPONSE_OK);
  dialog.set_default_response(Gtk::RESPONSE_OK);

  Gtk::Box box(Gtk::ORIENTATION_VERTICAL, 6);
  box.set_border_width(8);
  Gtk::Entry name;
  name.set_text(layer.name());
  Gtk::SpinButton opacity;
  opacity.set_range(0, 100);
  opacity.set_increments(1, 10);
  opacity.set_digits(0);
  opacity.set_value(static_cast<int>(layer.opacity() * 100.0f + 0.5f));
  Gtk::ComboBoxText blend;
  for (int i = 0; i < kBlendModeCount; ++i) {
    blend.append(blend_mode_label(blend_mode_from_index(i)));
  }
  blend.set_active(static_cast<int>(layer.blend()));
  box.pack_start(*Gtk::manage(new Gtk::Label("Name", 0.0, 0.5)), Gtk::PACK_SHRINK);
  box.pack_start(name, Gtk::PACK_SHRINK);
  box.pack_start(*Gtk::manage(new Gtk::Label("Opacity", 0.0, 0.5)), Gtk::PACK_SHRINK);
  box.pack_start(opacity, Gtk::PACK_SHRINK);
  box.pack_start(*Gtk::manage(new Gtk::Label("Blend", 0.0, 0.5)), Gtk::PACK_SHRINK);
  box.pack_start(blend, Gtk::PACK_SHRINK);
  dialog.get_content_area()->pack_start(box, Gtk::PACK_SHRINK);
  dialog.show_all();
  if (dialog.run() != Gtk::RESPONSE_OK) {
    return;
  }
  document_->rename_layer(index, name.get_text());
  document_->set_layer_opacity(index, static_cast<float>(opacity.get_value_as_int()) / 100.0f);
  document_->set_layer_blend(index, blend_mode_from_index(blend.get_active_row_number()));
}

void LayersPanel::update_buttons() {
  const bool have = document_ != nullptr && document_->layers().count() > 0;
  const int count = have ? document_->layers().count() : 0;
  const int active = have ? document_->layers().active_index() : 0;
  dup_.set_sensitive(have);
  del_.set_sensitive(have && count > 1);
  up_.set_sensitive(have && active + 1 < count);
  down_.set_sensitive(have && active > 0);
  merge_.set_sensitive(have && active > 0);
  flatten_.set_sensitive(have && count > 1);
  new_.set_sensitive(document_ != nullptr);
  blend_.set_sensitive(have);
}

void LayersPanel::refresh() {
  if (refreshing_) {
    return;
  }
  refreshing_ = true;
  std::vector<Gtk::Widget*> old;
  for (auto* child : list_.get_children()) {
    old.push_back(child);
  }
  for (auto* child : old) {
    list_.remove(*child);
  }
  if (document_ != nullptr) {
    for (int ui = 0; ui < document_->layers().count(); ++ui) {
      const int stack = document_->layers().count() - 1 - ui;
      add_row(stack);
    }
    const int active = document_->layers().active_index();
    const int ui = document_->layers().count() - 1 - active;
    if (auto* row = list_.get_row_at_index(ui)) {
      list_.select_row(*row);
    }
    if (document_->layers().count() > 0) {
      blend_.set_active(static_cast<int>(document_->layers().active_layer().blend()));
    }
  }
  list_.show_all();
  refreshing_ = false;
  update_buttons();
}

}  // namespace brushpad
