#!/bin/sh
# Adds Animal Crossing AP to your app launcher, for your user only (~/.local/share/applications).
# Only needed on Wayland, where the taskbar/dock icon comes from this entry; on X11 the game sets
# its own window icon. Run it from the game folder; run it again if you move the folder.
# Remove the entry with: ./install-desktop-entry.sh --remove
set -e
dir="$(cd "$(dirname "$0")" && pwd)"
apps="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
# The name matches the game's Wayland app id / X11 window class (set in pc_main.c)
file="$apps/AnimalCrossingAP.desktop"

if [ "$1" = "--remove" ]; then
  rm -f "$file"
  echo "Removed $file"
  exit 0
fi

mkdir -p "$apps"
cat > "$file" <<ENTRY
[Desktop Entry]
Type=Application
Name=Animal Crossing AP
Comment=Animal Crossing PC port with Archipelago
Exec="$dir/AnimalCrossing"
Path=$dir
Icon=$dir/icon.png
Terminal=false
Categories=Game;
StartupWMClass=AnimalCrossingAP
ENTRY
echo "Installed $file"
