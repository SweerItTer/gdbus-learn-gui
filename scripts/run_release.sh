#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-release"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_QT_GUI=ON
cmake --build "${BUILD_DIR}"

exec dbus-run-session -- bash -lc "
  cd \"${BUILD_DIR}\"
  ./training_service_gui_demo >/tmp/gui_service.log 2>&1 &
  server_pid=\$!
  trap 'kill \$server_pid; wait \$server_pid 2>/dev/null' EXIT
  ./sync_visualizer
"
