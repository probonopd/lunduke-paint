#!/bin/sh
set -e

# Setup AppDir structure with only the binary
rm -rf .AppDir
mkdir -p .AppDir/usr/bin
cp /tmp/lunduke-install/usr/bin/lunduke-paint .AppDir/usr/bin/

# Copy desktop file to root for AppRun
cp /tmp/lunduke-install/usr/share/applications/*.desktop .AppDir/
