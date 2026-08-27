# Brushpad

Brushpad is a traditional Linux X11 paint program built with GTK 3.24 and
gtkmm-3.0. Look and feel: classic MS Paint + KolourPaint, with Pinta-style
user layers.

- Application id: `org.brushpad.Brushpad`
- License: GPL-3.0-or-later
- Language: C++17
- Display: Linux X11 (`GDK_BACKEND=x11`)
- Chrome: window-manager title bar, menu bar, and toolbars (no client-side
  decorations, no HeaderBar as main chrome)
- Theme: follows the active GTK3 theme; the app is not skinned
- Native project file: OpenRaster `.ora` (libarchive + pugixml)

## Phase 4 is done

What works (Phase 1–4):

- One or more user layers (bottom → top in storage; Layers list is reversed
  so the top of the list is the top of the stack)
- Each layer: name, visible, locked, opacity 0–100%, blend, offset, RGBA8888
  buffer sized to the document, thumbnail cache
- Internal ToolLayer + SelectionLayer stay out of the Layers list
- Compositor draws the visible viewport only with v1 blends: Normal,
  Multiply, Screen, Overlay, Darken, Lighten
- Paint/effects still affect **active layer ∩ selection**
- Locked layer refuses pixel edits (status hint); hidden layers stay in the
  stack but are omitted from the composite
- Layers dock: thumbnail, name, eye, lock, opacity; blend combo; New /
  Duplicate / Delete / Up / Down / Merge down / Flatten; right-click + rename
- Layers menu: New (Ctrl+Shift+N), Duplicate, Delete, Raise, Lower, Merge
  down (Ctrl+E), Flatten (Ctrl+Shift+E), Properties
- Each layer op is one named, undoable Command
- OpenRaster `.ora` read/write (`mimetype` uncompressed first, `stack.xml`,
  `data/layer-N.png`, `mergedimage.png`; lock as `brushpad:locked`)
- Non-ORA files still open as a single layer named after the file
- Save As can pick ora/png/jpeg/bmp; multi-layer flat export warns and can
  also keep a `.ora`; never silently overwrites an `.ora` with a flat PNG
- JPEG/BMP flatten onto white and warn
- Multi-tab documents under the tool-options bar; File → Close (Ctrl+W);
  dirty tab close asks to save; New/Open add a tab (replace only an unused
  untitled placeholder)
- Per-document history; the active document drives canvas, layers, colors,
  undo
- Pencil, brush, eraser, fill, picker, rect select, line, rectangle, ellipse
- Color eraser, spraycan, rounded rect, polyline, polygon, curve, text
- Freeform (lasso) and ellipse select (same move / cut / copy / paste / delete / crop ops)
- History dock: named list, click an older row to undo there, a newer row to redo
- Transparent is a first-class FG/BG color (palette last cell + toolbox checker)
- Canvas size / scale / crop / rotate / flip apply to every user layer
- Headless tests: `dummy`, `fill`, `history`, `selection`, `transform`,
  `blend`, `ora`, `stroke`

Not in this phase: adjustments, effects, prefs dialog, print, About /
shortcuts window, extra packaging polish (Phase 5).

## Development

Development happens in the agent environment. This tree is the working
copy. Publishing to GitHub is a later joint step (see `PUBLISH.md`). You
do not need to install a toolchain on your own machine for day-to-day
work.

## How to build on X11 (Debian / Ubuntu)

```sh
sudo apt install -y build-essential meson ninja-build \
  libgtkmm-3.0-dev libcairomm-1.0-dev libgdk-pixbuf-2.0-dev \
  libarchive-dev libpugixml-dev

cd brushpad
meson setup build
meson compile -C build
meson test -C build
GDK_BACKEND=x11 ./build/brushpad
```

On Debian 13 the pixbuf development package is `libgdk-pixbuf-2.0-dev`.

Optional: after a clone, run the same commands on an X11 machine if you
want a local test-build. A missing `DISPLAY` is fine for compile and
`meson test`; the GUI is not required for Phase 4 checks.

## Decisions

- Flood-fill similarity is Chebyshev distance across RGBA (`max(|Δr|,|Δg|,|Δb|,|Δa|)`), matching classic Paint-style “how far from the seed color.”
- Eraser always paints the current background color (transparent BG punches alpha); both mouse buttons erase.
- JPEG and BMP flatten onto white (JPEG quality 90) and warn if the composite has transparency or more than one layer.
- PNG export from a multi-layer document flattens visible layers and keeps alpha, after a warning.
- Palette is an 8×6 Paint-like grid; the last swatch is transparent.
- History stores 32×32 tiles that actually changed inside the stroke dirty rect, not the whole layer.
- New canvas-resize pixels default to the current background color (Paint-like); the dialog also offers transparent.
- Image → Clear fills the active layer with the current background color (Paint-like).
- Delete/Backspace fills the selection with transparency, not the background.
- Moving a selection is preview-only until deselect; Ctrl during the move copies (original stays).
- Transparent move skips fully-transparent pixels while dragging and on drop; opaque move replaces the destination rect.
- Paste creates a floating selection on the active layer at the canvas origin.
- Crop to selection uses the rectangle bounds (insensitive for an inverted selection).
- Autocrop trims any outer row/column that is a single uniform color.
- Rotate 90 is clockwise.
- Scale dialog defaults to nearest neighbor and keep-aspect.
- Pixel grid is on by default and only drawn at zoom ≥ 400%.
- Blend onto a fully transparent destination uses the source color (Paint/Pinta-like; Multiply does not go black).
- Flatten discards hidden layers and composites the visible ones into one Background layer.
- Merge down blends the active layer onto the one below using the upper layer’s blend and opacity.
- Layer opacity in the dock commits on Enter or focus-out so a spin does not flood undo.
- Canvas size, scale, crop, rotate, and flip transform every user layer (KolourPaint/Pinta document ops).
- New/Open add a tab unless the only document is an unused untitled placeholder, which is replaced.
- Zoom and pan stay per window, not per tab.
- OpenRaster uses libarchive for the zip and pugixml for `stack.xml`.
- History dock starts with “New document”; later rows are command names; click jumps the per-document stack (not saved in `.ora`).
- Color eraser replaces FG-similar pixels (Chebyshev) with BG; right-drag swaps the roles.
- Spraycan drops a disk of random dots; density 1–100 is dots per stamp.
- Curve is the KolourPaint two-handle cubic: drag endpoints, then two control points.
- Polyline / polygon take click vertices; Enter or double-click finishes (polygon closes).
- Lasso and ellipse selections store an 8-bit mask; invert is still “canvas minus that region.”
- Text is a popup Entry; Pango rasterizes onto the layer on Enter or click-away (not a live object).
- Toolbox checker under the wells sets Transparent FG (left) or BG (right); painting with it punches alpha.

## Layout

Menu bar, main toolbar, tool-options bar (active tool only), document tabs
when more than one file is open, 2-column toolbox with FG/BG wells, canvas
with scrollbars, right dock (Colors / Layers / History), status bar
(`hint | x,y | sel w×h | canvas | zoom | modified`). F12 toggles the
right dock. History is a click-to-jump list for the active document.
