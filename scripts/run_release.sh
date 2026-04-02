#!/usr/bin/env bash
set -euo pipefail

# Build and run the Release GUI against an already-running system bus service.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-release"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_QT_GUI=ON
cmake --build "${BUILD_DIR}"

cd "${BUILD_DIR}"
exec ./sync_visualizer
