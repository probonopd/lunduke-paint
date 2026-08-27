project(
  'brushpad',
  'cpp',
  version: '0.0.0',
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

gnome = import('gnome')
brushpad_resources = gnome.compile_resources(
  'brushpad-resources',
  'data/brushpad.gresource.xml',
  source_dir: 'data',
  c_name: 'brushpad',
)

brushpad_inc = include_directories('src')

brushpad_sources = files(
  'src/main.cpp',
  'src/app/application.cpp',
  'src/app/main_window.cpp',
  'src/app/actions.cpp',
  'src/ui/canvas_view.cpp',
  'src/ui/toolbox.cpp',
  'src/ui/tool_options_bar.cpp',
  'src/ui/colors_panel.cpp',
  'src/ui/layers_panel.cpp',
  'src/ui/history_panel.cpp',
  'src/ui/status_bar.cpp',
  'src/ui/dialogs_new.cpp',
)

executable(
  'brushpad',
  brushpad_sources,
  brushpad_resources,
  dependencies: [gtkmm_dep, cairomm_dep],
  include_directories: brushpad_inc,
  install: true,
)

test_dummy = executable(
  'test_dummy',
  files('tests/test_dummy.cpp'),
  install: false,
)

test('dummy', test_dummy)

install_data(
  'data/org.brushpad.Brushpad.desktop',
  install_dir: join_paths(get_option('datadir'), 'applications'),
)

install_data(
  'data/org.brushpad.Brushpad.metainfo.xml',
  install_dir: join_paths(get_option('datadir'), 'metainfo'),
)

install_data(
  'data/icons/hicolor/scalable/apps/org.brushpad.Brushpad.svg',
  install_dir: join_paths(get_option('datadir'), 'icons/hicolor/scalable/apps'),
)
