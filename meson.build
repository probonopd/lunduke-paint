project(
  'brushpad',
  'cpp',
  version: '1.0.0',
  license: 'GPL-3.0-or-later',
  meson_version: '>=0.56.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
    'werror=false',
  ],
)

# warning_level=2 => -Wall -Wextra
gtkmm_dep = dependency('gtkmm-3.0', version: '>=3.24')
cairomm_dep = dependency('cairomm-1.0')
gdkpixbuf_dep = dependency('gdk-pixbuf-2.0')
libarchive_dep = dependency('libarchive')
pugixml_dep = dependency('pugixml')

gnome = import('gnome')
brushpad_resources = gnome.compile_resources(
  'brushpad-resources',
  'data/brushpad.gresource.xml',
  source_dir: 'data',
  c_name: 'brushpad',
)

brushpad_inc = include_directories('src')

brushpad_core_sources = files(
  'src/doc/document.cpp',
  'src/doc/workspace.cpp',
  'src/doc/layer.cpp',
  'src/doc/layer_stack.cpp',
  'src/doc/history.cpp',
  'src/doc/commands_pixels.cpp',
  'src/doc/commands_image.cpp',
  'src/doc/selection.cpp',
  'src/raster/fill.cpp',
  'src/raster/stroke.cpp',
  'src/raster/transform.cpp',
  'src/raster/shapes.cpp',
  'src/raster/blend.cpp',
  'src/raster/text.cpp',
  'src/raster/effects.cpp',
  'src/doc/commands_layers.cpp',
  'src/io/image_io.cpp',
  'src/io/ora.cpp',
)

brushpad_core = static_library(
  'brushpad_core',
  brushpad_core_sources,
  include_directories: brushpad_inc,
  dependencies: [gdkpixbuf_dep, libarchive_dep, pugixml_dep],
)

brushpad_sources = files(
  'src/main.cpp',
  'src/app/application.cpp',
  'src/app/main_window.cpp',
  'src/app/actions.cpp',
  'src/app/preferences.cpp',
  'src/ui/canvas_view.cpp',
  'src/ui/toolbox.cpp',
  'src/ui/tool_options_bar.cpp',
  'src/ui/colors_panel.cpp',
  'src/ui/layers_panel.cpp',
  'src/ui/history_panel.cpp',
  'src/ui/status_bar.cpp',
  'src/ui/dialogs_new.cpp',
  'src/tools/tool.cpp',
  'src/tools/tool_pencil.cpp',
  'src/tools/tool_brush.cpp',
  'src/tools/tool_eraser.cpp',
  'src/tools/tool_picker.cpp',
  'src/tools/tool_fill.cpp',
  'src/tools/tool_rect_select.cpp',
  'src/tools/tool_line.cpp',
  'src/tools/tool_rectangle.cpp',
  'src/tools/tool_ellipse.cpp',
  'src/tools/tool_color_eraser.cpp',
  'src/tools/tool_spray.cpp',
  'src/tools/tool_rounded_rect.cpp',
  'src/tools/tool_polyline.cpp',
  'src/tools/tool_polygon.cpp',
  'src/tools/tool_curve.cpp',
  'src/tools/tool_lasso.cpp',
  'src/tools/tool_ellipse_select.cpp',
  'src/tools/tool_text.cpp',
  'src/ui/dialogs_image.cpp',
  'src/ui/dialogs_adjust.cpp',
  'src/ui/dialogs_prefs.cpp',
)

executable(
  'lunduke-paint',
  brushpad_sources,
  brushpad_resources,
  dependencies: [gtkmm_dep, cairomm_dep, gdkpixbuf_dep],
  include_directories: brushpad_inc,
  link_with: brushpad_core,
  install: true,
)

test_dummy = executable(
  'test_dummy',
  files('tests/test_dummy.cpp'),
  install: false,
)
test('dummy', test_dummy)

test_fill = executable(
  'test_fill',
  files('tests/test_fill.cpp'),
  include_directories: brushpad_inc,
  link_with: brushpad_core,
  install: false,
)
test('fill', test_fill)

test_history = executable(
  'test_history',
  files('tests/test_history.cpp'),
  include_directories: brushpad_inc,
  link_with: brushpad_core,
  install: false,
)
test('history', test_history)

test_selection = executable(
  'test_selection',
  files('tests/test_selection.cpp'),
  include_directories: brushpad_inc,
  link_with: brushpad_core,
  install: false,
)
test('selection', test_selection)

test_transform = executable(
  'test_transform',
  files('tests/test_transform.cpp'),
  include_directories: brushpad_inc,
  link_with: brushpad_core,
  install: false,
)
test('transform', test_transform)

test_blend = executable(
  'test_blend',
  files('tests/test_blend.cpp'),
  include_directories: brushpad_inc,
  link_with: brushpad_core,
  install: false,
)
test('blend', test_blend)

test_ora = executable(
  'test_ora',
  files('tests/test_ora.cpp'),
  include_directories: brushpad_inc,
  link_with: brushpad_core,
  dependencies: [gdkpixbuf_dep, libarchive_dep, pugixml_dep],
  install: false,
)
test('ora', test_ora)

test_stroke = executable(
  'test_stroke',
  files('tests/test_stroke.cpp'),
  include_directories: brushpad_inc,
  link_with: brushpad_core,
  install: false,
)
test('stroke', test_stroke)

test_effects = executable(
  'test_effects',
  files('tests/test_effects.cpp'),
  include_directories: brushpad_inc,
  link_with: brushpad_core,
  install: false,
)
test('effects', test_effects)

install_data(
  'data/org.lunduke.LundukePaint.desktop',
  install_dir: join_paths(get_option('datadir'), 'applications'),
)

install_data(
  'data/org.lunduke.LundukePaint.metainfo.xml',
  install_dir: join_paths(get_option('datadir'), 'metainfo'),
)

install_data(
  'data/icons/hicolor/scalable/apps/org.lunduke.LundukePaint.svg',
  install_dir: join_paths(get_option('datadir'), 'icons/hicolor/scalable/apps'),
)
