# Brushpad

Brushpad is a traditional Linux X11 paint program built with GTK 3.24 and
gtkmm-3.0. Phase 0 is an application shell: window chrome, a white canvas
view that reports mouse coordinates, and File actions. Painting, layers,
and tools are not implemented yet.

- Application id: `org.brushpad.Brushpad`
- License: GPL-3.0-or-later
- Language: C++17
- Display: Linux X11 (`GDK_BACKEND=x11`)
- Chrome: window-manager title bar, menu bar, and toolbars (no client-side
  decorations, no HeaderBar as main chrome)
- Theme: follows the active GTK3 theme; the app is not skinned

## Development

Development happens in the agent environment. This tree is the working
copy. Publishing to GitHub is a later joint step (see `PUBLISH.md`). You
do not need to install a toolchain on your own machine for day-to-day
work.

## How to build on X11 (Debian / Ubuntu)

```sh
sudo apt install -y build-essential meson ninja-build \
  libgtkmm-3.0-dev libcairomm-1.0-dev libgdk-pixbuf-2.0-dev \
  libarchive-dev libpugixml-dev libcatch2-dev

cd brushpad
meson setup build
meson compile -C build
meson test -C build
GDK_BACKEND=x11 ./build/brushpad
```

On Debian 13 the pixbuf development package is `libgdk-pixbuf-2.0-dev`.

Optional: after a clone, run the same commands on an X11 machine if you
want a local test-build. A missing `DISPLAY` is fine for compile and
`meson test`; the GUI is not required for Phase 0 checks.

## Layout (Phase 0)

Menu bar, main toolbar (empty), tool-options bar (empty), left tools
column (empty), white canvas, right dock (Colors / Layers / History
tabs, empty), status bar. F12 toggles the right dock.

Wired actions: New (blank canvas), Open (stub), Save (stub), Quit.
