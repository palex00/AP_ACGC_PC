#!/bin/sh

# haloopdy 2026
# Options are passed to build_mingw.sh (e.g. --clean to remove compiled outputs first).
# THIS is the file to run if you want to build! Should work on any system with podman installed?

# Host networking avoids pasta, which fails on some hosts (e.g. Ubuntu AppArmor). The build needs
# network for apt and the CMake cacert download, and never listens on anything.
podman build --network=host -t ap-acgc-build . || exit 1
podman run --rm --network=host -v .:/build:Z --userns=keep-id -e HOST_DIR="$PWD" ap-acgc-build ./build_mingw.sh "$@"
