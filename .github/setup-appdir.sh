#!/bin/sh
set -e

# Setup AppDir structure
rm -rf .AppDir
mkdir -p .AppDir/usr/bin
mkdir -p .AppDir/usr/share/applications
mkdir -p .AppDir/usr/share/metainfo
mkdir -p .AppDir/usr/share/icons/hicolor
mkdir -p .AppDir/usr/share/mime/packages
mkdir -p .AppDir/usr/share/gtk-3.0

# Copy installed files
cp /tmp/lunduke-install/usr/bin/lunduke-paint .AppDir/usr/bin/
cp /tmp/lunduke-install/usr/share/applications/*.desktop .AppDir/usr/share/applications/
cp /tmp/lunduke-install/usr/share/metainfo/*.xml .AppDir/usr/share/metainfo/
cp -r /tmp/lunduke-install/usr/share/icons/hicolor/* .AppDir/usr/share/icons/hicolor/
cp /tmp/lunduke-install/usr/share/mime/packages/*.xml .AppDir/usr/share/mime/packages/
cp -r /tmp/lunduke-install/usr/share/gtk-3.0/* .AppDir/usr/share/gtk-3.0/ 2>/dev/null || true

# Copy hicolor icon theme index from system
if [ -d /usr/share/icons/hicolor ]; then
    cp /usr/share/icons/hicolor/index.theme .AppDir/usr/share/icons/hicolor/ 2>/dev/null || true
fi

# Copy GTK UI files from system (needed for GtkBuilder templates)
if [ -d /usr/share/gtk-3.0 ]; then
    for dir in /usr/share/gtk-3.0/*/; do
        [ -d "$dir" ] || continue
        dirname=$(basename "$dir")
        mkdir -p ".AppDir/usr/share/gtk-3.0/$dirname"
        cp -r "$dir"*.ui ".AppDir/usr/share/gtk-3.0/$dirname/" 2>/dev/null || true
    done
fi

# Regenerate gdk-pixbuf loaders.cache to include all bundled loaders
if command -v gdk-pixbuf-query-loaders >/dev/null 2>&1; then
    find .AppDir -name "loaders" -type d -path "*/gdk-pixbuf-2.0/*" -exec sh -c '
        for d in "$@"; do
            cache="${d}/loaders.cache"
            gdk-pixbuf-query-loaders "$d"/*.so > "$cache" 2>/dev/null || true
        done
    ' _ {} +
fi

# Copy desktop file to root for AppRun
cp .AppDir/usr/share/applications/*.desktop .AppDir/
