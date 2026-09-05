#!/bin/sh
set -e

# Create minimal AppDir with just the binary and desktop file
rm -rf AppDir
mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/share/applications
cp /tmp/lunduke-install/usr/bin/lunduke-paint AppDir/usr/bin/
cp /tmp/lunduke-install/usr/share/applications/*.desktop AppDir/usr/share/applications/
