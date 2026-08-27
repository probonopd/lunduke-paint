// SPDX-License-Identifier: GPL-3.0-or-later

#include "io/ora.hpp"

#include "doc/document.hpp"
#include "io/image_io.hpp"
#include "raster/blend.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <pugixml.hpp>

#include <algorithm>
#include <cstring>
#include <map>
#include <sstream>

namespace brushpad {
namespace {

constexpr const char* kMimetype = "image/openraster";
constexpr const char* kBrushpadNs = "http://brushpad.org/ns";

struct ZipEntry {
  std::string path;
  std::vector<std::uint8_t> data;
};

std::string archive_err(archive* a, const char* fallback) {
  const char* msg = archive_error_string(a);
  return msg != nullptr ? msg : fallback;
}

bool write_zip_entry(archive* a, const char* path, const void* data, std::size_t size) {
  archive_entry* entry = archive_entry_new();
  if (entry == nullptr) {
    return false;
  }
  archive_entry_set_pathname(entry, path);
  archive_entry_set_size(entry, static_cast<la_int64_t>(size));
  archive_entry_set_filetype(entry, AE_IFREG);
  archive_entry_set_perm(entry, 0644);
  if (archive_write_header(a, entry) != ARCHIVE_OK) {
    archive_entry_free(entry);
    return false;
  }
  if (size > 0 && archive_write_data(a, data, size) != static_cast<la_ssize_t>(size)) {
    archive_entry_free(entry);
    return false;
  }
  archive_entry_free(entry);
  return true;
}

bool read_zip(const std::string& path, std::map<std::string, std::vector<std::uint8_t>>& files,
              std::string& error) {
  archive* a = archive_read_new();
  if (a == nullptr) {
    error = "Could not allocate archive reader";
    return false;
  }
  archive_read_support_format_zip(a);
  archive_read_support_filter_all(a);
  if (archive_read_open_filename(a, path.c_str(), 16384) != ARCHIVE_OK) {
    error = archive_err(a, "Could not open OpenRaster file");
    archive_read_free(a);
    return false;
  }
  archive_entry* entry = nullptr;
  while (true) {
    const int r = archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF) {
      break;
    }
    if (r != ARCHIVE_OK) {
      error = archive_err(a, "Corrupt or truncated OpenRaster file");
      archive_read_free(a);
      return false;
    }
    const char* name = archive_entry_pathname(entry);
    if (name == nullptr) {
      continue;
    }
    if (archive_entry_filetype(entry) != AE_IFREG) {
      archive_read_data_skip(a);
      continue;
    }
    std::vector<std::uint8_t> data;
    const la_int64_t sz = archive_entry_size(entry);
    if (sz > 0) {
      data.resize(static_cast<std::size_t>(sz));
      std::size_t got = 0;
      while (got < data.size()) {
        const la_ssize_t n = archive_read_data(a, data.data() + got, data.size() - got);
        if (n <= 0) {
          error = "Corrupt or truncated OpenRaster file";
          archive_read_free(a);
          return false;
        }
        got += static_cast<std::size_t>(n);
      }
    } else {
      std::uint8_t buf[4096];
      while (true) {
        const la_ssize_t n = archive_read_data(a, buf, sizeof(buf));
        if (n == 0) {
          break;
        }
        if (n < 0) {
          error = "Corrupt or truncated OpenRaster file";
          archive_read_free(a);
          return false;
        }
        data.insert(data.end(), buf, buf + n);
      }
    }
    files[name] = std::move(data);
  }
  archive_read_free(a);
  return true;
}

std::string make_stack_xml(const Document& document) {
  pugi::xml_document xml;
  auto decl = xml.prepend_child(pugi::node_declaration);
  decl.append_attribute("version") = "1.0";
  decl.append_attribute("encoding") = "UTF-8";
  auto image = xml.append_child("image");
  image.append_attribute("version") = "0.0.3";
  image.append_attribute("w") = document.width();
  image.append_attribute("h") = document.height();
  image.append_attribute("xmlns:brushpad") = kBrushpadNs;
  auto stack = image.append_child("stack");
  for (int i = document.layers().count() - 1; i >= 0; --i) {
    const Layer& layer = document.layers().at(i);
    auto node = stack.append_child("layer");
    node.append_attribute("name") = layer.name().c_str();
    const std::string src = "data/layer-" + std::to_string(i) + ".png";
    node.append_attribute("src") = src.c_str();
    node.append_attribute("x") = layer.offset_x();
    node.append_attribute("y") = layer.offset_y();
    node.append_attribute("opacity") = layer.opacity();
    node.append_attribute("visibility") = layer.visible() ? "visible" : "hidden";
    node.append_attribute("composite-op") = blend_mode_ora_op(layer.blend());
    node.append_attribute("brushpad:locked") = layer.locked() ? "true" : "false";
  }
  std::ostringstream out;
  xml.save(out, "  ");
  return out.str();
}

}  // namespace

bool save_ora(const std::string& path, const Document& document, std::string& error) {
  if (document.width() < 1 || document.height() < 1 || document.layers().count() < 1) {
    error = "Nothing to save";
    return false;
  }
  archive* a = archive_write_new();
  if (a == nullptr) {
    error = "Could not allocate archive writer";
    return false;
  }
  archive_write_set_format_zip(a);
  if (archive_write_open_filename(a, path.c_str()) != ARCHIVE_OK) {
    error = archive_err(a, "Could not create OpenRaster file");
    archive_write_free(a);
    return false;
  }

  archive_write_zip_set_compression_store(a);
  if (!write_zip_entry(a, "mimetype", kMimetype, std::strlen(kMimetype))) {
    error = archive_err(a, "Could not write mimetype");
    archive_write_free(a);
    return false;
  }
  archive_write_zip_set_compression_deflate(a);

  const std::string stack_xml = make_stack_xml(document);
  if (!write_zip_entry(a, "stack.xml", stack_xml.data(), stack_xml.size())) {
    error = archive_err(a, "Could not write stack.xml");
    archive_write_free(a);
    return false;
  }

  for (int i = 0; i < document.layers().count(); ++i) {
    const Layer& layer = document.layers().at(i);
    std::vector<std::uint8_t> png;
    if (!encode_png_memory(layer.pixels(), layer.width(), layer.height(), layer.stride(), png,
                           error)) {
      archive_write_free(a);
      return false;
    }
    const std::string name = "data/layer-" + std::to_string(i) + ".png";
    if (!write_zip_entry(a, name.c_str(), png.data(), png.size())) {
      error = archive_err(a, "Could not write layer PNG");
      archive_write_free(a);
      return false;
    }
  }

  std::vector<std::uint8_t> merged(
      static_cast<std::size_t>(document.width()) * static_cast<std::size_t>(document.height()) * 4,
      0);
  document.layers().composite_rect(merged.data(), document.width() * 4,
                                   Rect{0, 0, document.width(), document.height()});
  std::vector<std::uint8_t> merged_png;
  if (!encode_png_memory(merged.data(), document.width(), document.height(), document.width() * 4,
                         merged_png, error)) {
    archive_write_free(a);
    return false;
  }
  if (!write_zip_entry(a, "mergedimage.png", merged_png.data(), merged_png.size())) {
    error = archive_err(a, "Could not write mergedimage.png");
    archive_write_free(a);
    return false;
  }

  if (archive_write_close(a) != ARCHIVE_OK) {
    error = archive_err(a, "Could not finish OpenRaster file");
    archive_write_free(a);
    return false;
  }
  archive_write_free(a);
  return true;
}

LoadedOra load_ora(const std::string& path) {
  LoadedOra out;
  std::map<std::string, std::vector<std::uint8_t>> files;
  if (!read_zip(path, files, out.error)) {
    return out;
  }
  auto mit = files.find("mimetype");
  if (mit == files.end() ||
      std::string(mit->second.begin(), mit->second.end()) != kMimetype) {
    out.error = "Not an OpenRaster file (missing image/openraster mimetype)";
    return out;
  }
  auto sit = files.find("stack.xml");
  if (sit == files.end()) {
    out.error = "OpenRaster file is missing stack.xml";
    return out;
  }
  pugi::xml_document xml;
  const pugi::xml_parse_result parsed =
      xml.load_buffer(sit->second.data(), sit->second.size());
  if (!parsed) {
    out.error = "Corrupt stack.xml";
    return out;
  }
  auto image = xml.child("image");
  if (!image) {
    out.error = "stack.xml is missing the image element";
    return out;
  }
  out.width = image.attribute("w").as_int();
  out.height = image.attribute("h").as_int();
  if (out.width < 1 || out.height < 1) {
    out.error = "Invalid OpenRaster canvas size";
    return out;
  }
  if (out.width > kHardMaxSide || out.height > kHardMaxSide) {
    out.error = "Image is larger than 16384 on a side";
    return out;
  }
  if (out.width > kSoftMaxSide || out.height > kSoftMaxSide) {
    out.warn_size = true;
  }

  std::vector<LayerSnapshot> top_to_bottom;
  auto stack = image.child("stack");
  for (auto node : stack.children("layer")) {
    LayerSnapshot snap;
    snap.name = node.attribute("name").as_string("Layer");
    snap.offset_x = node.attribute("x").as_int(0);
    snap.offset_y = node.attribute("y").as_int(0);
    snap.opacity = node.attribute("opacity").as_float(1.0f);
    const char* vis = node.attribute("visibility").as_string("visible");
    snap.visible = std::strcmp(vis, "hidden") != 0 && std::strcmp(vis, "false") != 0;
    snap.blend = blend_mode_from_ora(node.attribute("composite-op").as_string("svg:src-over"));
    const char* locked = node.attribute("locked").as_string(nullptr);
    if (locked == nullptr) {
      locked = node.attribute("brushpad:locked").as_string("false");
    }
    snap.locked = std::strcmp(locked, "true") == 0 || std::strcmp(locked, "1") == 0;
    const char* src = node.attribute("src").as_string("");
    auto pit = files.find(src);
    if (pit == files.end()) {
      out.error = std::string("OpenRaster is missing layer image ") + src;
      return out;
    }
    LoadedImage png;
    if (!decode_png_memory(pit->second.data(), pit->second.size(), png)) {
      out.error = png.error.empty() ? "Corrupt layer PNG" : png.error;
      return out;
    }
    snap.width = png.width;
    snap.height = png.height;
    snap.pixels = std::move(png.rgba);
    top_to_bottom.push_back(std::move(snap));
  }
  if (top_to_bottom.empty()) {
    out.error = "OpenRaster file has no layers";
    return out;
  }
  if (static_cast<int>(top_to_bottom.size()) > kSoftMaxLayers) {
    out.warn_layers = true;
  }
  out.layers.assign(top_to_bottom.rbegin(), top_to_bottom.rend());
  return out;
}

}  // namespace brushpad
