# Brushpad

Brushpad is a traditional Linux X11 paint program built with GTK 3.24 and
gtkmm-3.0. Look and feel: classic MS Paint + KolourPaint.

- Application id: `org.brushpad.Brushpad`
- License: GPL-3.0-or-later
- Language: C++17
- Display: Linux X11 (`GDK_BACKEND=x11`)
- Chrome: window-manager title bar, menu bar, and toolbars (no client-side
  decorations, no HeaderBar as main chrome)
- Theme: follows the active GTK3 theme; the app is not skinned

## Phase 2 is done

What works (Phase 1 plus Phase 2):

- One-layer document (800×600 white by default), New dialog (size + white /
  transparent / custom color)
- Pencil (hard pixels), round brush (optional anti-alias), eraser, flood fill
  with tolerance, color picker (returns to the previous tool)
- Rect select (S), marching ants, move, Ctrl-stamp/copy, cut/copy/paste/delete,
  Select All / Deselect / Invert, Duplicate (Ctrl+J and Ctrl-drag)
- Line (L), rectangle (R), ellipse (E): Shift constrains, fill/stroke/both,
  thickness, optional AA; rasterize on mouse-up
- Left button = foreground, right-drag = background; no right-click menu
- Paint/fill clip to the current selection (active layer ∩ selection)
- Canvas size, scale (nearest + bilinear), crop to selection, autocrop,
  rotate 90/180, flip H/V, clear
- Pixel grid at zoom ≥ 400% (View → Grid / Ctrl+G)
- Integer zoom-in stays nearest neighbor
- Zoom in/out/100%/fit (Ctrl++ / Ctrl+- / Ctrl+0 / Ctrl+1)
- FG/BG wells, X swap, D reset, static 48-color palette (including transparent)
- Live stroke on a ToolLayer; floating selection on a SelectionLayer
- Dirty-rect / changed-tile history for strokes; full-buffer snapshot for
  rotate/flip/scale/canvas-size
- Zoom (Ctrl+wheel toward pointer), pan (middle button or Space+drag),
  scrollbars, checkerboard behind transparent pixels
- Open / Save / Save As PNG, JPEG, BMP via GdkPixbuf; dirty title asterisk
- Toolbar: New Open Save | Cut Copy Paste | Undo Redo | Zoom | F12
- Status bar shows `sel w×h` when a selection exists
- Headless tests: `dummy`, `fill`, `history`, `selection`, `transform`

Not in this phase: multi-layer UI, ORA, text, history dock wiring, spray /
polygon / curve / lasso / ellipse select.

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
`meson test`; the GUI is not required for Phase 2 checks.

## Decisions

- Flood-fill similarity is Chebyshev distance across RGBA (`max(|Δr|,|Δg|,|Δb|,|Δa|)`), matching classic Paint-style “how far from the seed color.”
- Eraser always paints the current background color (transparent BG punches alpha); both mouse buttons erase.
- JPEG and BMP flatten onto white (JPEG quality 90) and warn if the layer has transparency.
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

## Layout

Menu bar, main toolbar, tool-options bar (active tool only), 2-column toolbox
with FG/BG wells, canvas with scrollbars, right dock (Colors / Layers /
History), status bar (`hint | x,y | sel w×h | canvas | zoom | modified`). F12
toggles the right dock. Layers and History tabs are placeholders until later
phases.
