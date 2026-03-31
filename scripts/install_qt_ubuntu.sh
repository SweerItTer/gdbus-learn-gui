#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  dbus-x11 \
  libglib2.0-dev \
  libglib2.0-dev-bin \
  qt6-base-dev \
  qt6-base-dev-tools \
  qt6-tools-dev-tools
