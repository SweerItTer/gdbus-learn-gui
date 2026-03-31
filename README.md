# GUI Sync Visualizer

Qt Widgets demo for `/home/mouj/projects/gdbus-test` that replaces the CLI client with a visual, two-window synchronization test tool.

## Features

- Opens two independent windows, `Window A` and `Window B`.
- Maps one `TestInfo` object to a synchronized widget pair:
  - `QCheckBox` for `bool_param`
  - draggable `QLineEdit` for `string_param`
- Keeps relative control positioning stable while windows scale.
- Uses `int_param + double_param` as the logical drag coordinate.
- Commits the final drag position by rounding `int_param` and clearing `double_param`.
- Reuses `TrainingClient` directly and now handles remote updates through inherited callback overrides instead of polling.
- Supports file selection and file sending through `TrainingClient::SendFileByPath(...)`.
- Shows a dedicated file-transfer result label so success or failure is explicit.

## Project Layout

- [CMakeLists.txt](/home/mouj/projects/gui/CMakeLists.txt): build entrypoint
- [main.cpp](/home/mouj/projects/gui/src/main.cpp): app startup, opens A/B windows
- [MainWindow.cpp](/home/mouj/projects/gui/src/MainWindow.cpp): top-level UI, file picker, send button, status labels
- [TestInfoCanvas.cpp](/home/mouj/projects/gui/src/TestInfoCanvas.cpp): checkbox, editable field, drag behavior
- [TrainingClientBridge.cpp](/home/mouj/projects/gui/src/TrainingClientBridge.cpp): `QObject + TrainingClient` bridge
- [SyncMath.cpp](/home/mouj/projects/gui/src/SyncMath.cpp): coordinate mapping and scaling helpers
- [FileTransferState.cpp](/home/mouj/projects/gui/src/FileTransferState.cpp): selected-file state helper

## Requirements

Ubuntu 22.04 / WSL2 packages:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  dbus-x11 \
  libglib2.0-dev \
  libglib2.0-dev-bin \
  libgl1-mesa-dev \
  qt6-base-dev \
  qt6-base-dev-tools \
  qt6-tools-dev-tools
```

Or run:

```bash
./scripts/install_qt_ubuntu.sh
```

You also need a working desktop display:

- WSLg on Windows 11, or
- an X11 server with a valid `DISPLAY`

## Build

Debug or default build:

```bash
cmake -S . -B build-gui -G Ninja -DENABLE_QT_GUI=ON
cmake --build build-gui
```

Release build:

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_QT_GUI=ON
cmake --build build-release
```

Logic-only test build:

```bash
cmake -S . -B build-test -G Ninja -DENABLE_QT_GUI=OFF
cmake --build build-test
ctest --test-dir build-test --output-on-failure
```

## Run

Manual run from a built directory:

```bash
dbus-run-session -- bash -lc './build-release/training_service_gui_demo & server_pid=$!; trap "kill $server_pid; wait $server_pid 2>/dev/null" EXIT; ./build-release/sync_visualizer'
```

One-command release run:

```bash
./scripts/run_release.sh
```

## How To Verify

1. Toggle the checkbox in either window and verify the input field shows or hides in both windows.
2. Edit the text in either window and verify the other window updates.
3. Drag the input field in one window and verify the other window follows.
4. Release the drag and verify the final position snaps to integer alignment.
5. Use `Select File`, then `Send File`, and verify the file-transfer result label reports success or failure clearly.

## Verified Locally

- Debug GUI build succeeded.
- Release GUI build will be produced by `./scripts/run_release.sh`.
- `ctest --test-dir build-gui --output-on-failure` passed.
- Offscreen startup with `dbus-run-session` succeeded for both service and GUI.
