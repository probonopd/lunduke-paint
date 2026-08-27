// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/history_panel.hpp"

#include "doc/document.hpp"

#include <gtkmm/label.h>
#include <gtkmm/listboxrow.h>

namespace brushpad {
namespace {

class HistoryRow : public Gtk::ListBoxRow {
public:
  HistoryRow(int history_index, const Glib::ustring& name, bool current, bool future)
      : history_index(history_index) {
    auto* label = Gtk::manage(new Gtk::Label(name));
    label->set_xalign(0.0f);
    label->set_ellipsize(Pango::ELLIPSIZE_END);
    if (future) {
      label->set_opacity(0.45);
    }
    add(*label);
    if (current) {
      get_style_context()->add_class("suggested-action");
    }
    show_all();
  }

  int history_index = -1;
};

}  // namespace

HistoryPanel::HistoryPanel() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4) {
  set_border_width(8);
  list_.set_selection_mode(Gtk::SELECTION_SINGLE);
  list_.set_activate_on_single_click(true);
  list_.signal_row_activated().connect(sigc::mem_fun(*this, &HistoryPanel::on_row_activated));
  scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  scroll_.set_min_content_height(160);
  scroll_.add(list_);
  pack_start(scroll_, Gtk::PACK_EXPAND_WIDGET);
}

void HistoryPanel::set_document(Document* document) {
  document_ = document;
  refresh();
}

void HistoryPanel::refresh() {
  refreshing_ = true;
  const std::vector<Gtk::Widget*> children = list_.get_children();
  for (Gtk::Widget* child : children) {
    list_.remove(*child);
  }

  const int current = document_ != nullptr ? document_->history().index() : -1;
  const int count = document_ != nullptr ? document_->history().count() : 0;

  auto* initial = Gtk::manage(new HistoryRow(-1, "New document", current == -1, false));
  list_.append(*initial);

  for (int i = 0; i < count; ++i) {
    const bool is_current = (i == current);
    const bool future = (i > current);
    auto* row = Gtk::manage(new HistoryRow(i, document_->history().name_at(i), is_current, future));
    list_.append(*row);
  }

  list_.show_all();
  const int select = current + 1;  // row 0 is "New document"
  if (Gtk::ListBoxRow* row = list_.get_row_at_index(select)) {
    list_.select_row(*row);
  }
  refreshing_ = false;
}

void HistoryPanel::on_row_activated(Gtk::ListBoxRow* row) {
  if (refreshing_ || row == nullptr || !on_jump) {
    return;
  }
  auto* hrow = dynamic_cast<HistoryRow*>(row);
  if (hrow == nullptr) {
    return;
  }
  on_jump(hrow->history_index);
}

}  // namespace brushpad
