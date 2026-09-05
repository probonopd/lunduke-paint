#!/bin/sh
set -e

# Create AppDir with binary, desktop file, and icons
rm -rf AppDir
mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/share/applications
mkdir -p AppDir/usr/share/icons/hicolor/32x32/apps
mkdir -p AppDir/usr/share/icons/hicolor/48x48/apps
mkdir -p AppDir/usr/share/icons/hicolor/96x96/apps
mkdir -p AppDir/usr/share/icons/hicolor/scalable/apps

cp /tmp/lunduke-install/usr/bin/lunduke-paint AppDir/usr/bin/
cp /tmp/lunduke-install/usr/share/applications/*.desktop AppDir/usr/share/applications/
cp /tmp/lunduke-install/usr/share/icons/hicolor/32x32/apps/*.png AppDir/usr/share/icons/hicolor/32x32/apps/
cp /tmp/lunduke-install/usr/share/icons/hicolor/48x48/apps/*.png AppDir/usr/share/icons/hicolor/48x48/apps/
cp /tmp/lunduke-install/usr/share/icons/hicolor/96x96/apps/*.png AppDir/usr/share/icons/hicolor/96x96/apps/
cp /tmp/lunduke-install/usr/share/icons/hicolor/scalable/apps/*.svg AppDir/usr/share/icons/hicolor/scalable/apps/
