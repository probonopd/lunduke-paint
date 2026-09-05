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

# Copy hicolor icon theme index from system if present
if [ -d /usr/share/icons/hicolor ]; then
    cp /usr/share/icons/hicolor/index.theme .AppDir/usr/share/icons/hicolor/ 2>/dev/null || true
fi

# Copy desktop file to root for AppRun
cp .AppDir/usr/share/applications/*.desktop .AppDir/
