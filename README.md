# gdbus-learn-gui

一个基于 `Qt Widgets` 的图形化联调工程，用来给 [`gdbus-learn`](https://github.com/SweerItTer/gdbus-learn) 提供可视化客户端，替代原来的命令行交互方式。

当前项目主要用于验证以下能力：

- 使用 `Qt Widgets` 构建双窗口同步演示界面
- 基于 `TrainingClient` 与 `gdbus-learn` 的 `service` / `client` 通信链路对接
- 使用 `FetchContent` 从远端仓库拉取 `gdbus-learn` 源码完成构建
- 实时展示 `TestInfo` 的勾选、文本、拖拽位置同步
- 使用现有文件发送接口进行文件选择与发送，并在界面上明确显示结果

当前项目已经实现了：

- 同时打开 `Window A` / `Window B` 两个窗口
- `QCheckBox` 控制输入框显示或隐藏
- `QLineEdit` 文本编辑双窗口同步
- 拖拽输入框时基于 `int_param + double_param` 做位置同步
- 拖拽结束后按整数位置对齐
- 通过继承 `TrainingClient` 重写远端回调，去掉原先轮询模式
- 通过 `SendFileByPath` 发送文件
- 单独显示文件发送成功 / 失败结果
- 一键 `Release` 构建并启动演示

## 目录结构

```text
gdbus-learn-gui/
├── src/
│   ├── main.cpp
│   ├── MainWindow.*
│   ├── TestInfoCanvas.*
│   ├── TrainingClientBridge.*
│   ├── SyncMath.*
│   └── FileTransferState.*
├── tests/
│   └── sync_math_tests.cpp
├── scripts/
│   ├── install_qt_ubuntu.sh
│   └── run_release.sh
├── CMakeLists.txt
└── README.md
```

各部分职责如下：

- `src/MainWindow.*`
  顶层窗口，负责状态展示、文件选择、发送按钮和整体布局。
- `src/TestInfoCanvas.*`
  自定义同步控件，包含勾选框、输入框和拖拽处理逻辑。
- `src/TrainingClientBridge.*`
  `QObject + TrainingClient` 桥接层，负责把远端回调转换成 Qt 信号，并向后端提交变更。
- `src/SyncMath.*`
  负责缩放、坐标换算、拖拽预览和拖拽提交时的数学逻辑。
- `src/FileTransferState.*`
  负责记录当前选择的文件路径和发送前状态。
- `scripts/install_qt_ubuntu.sh`
  Ubuntu / WSL2 下安装 Qt 和构建依赖。
- `scripts/run_release.sh`
  一键 `Release` 构建并启动 `service + GUI`。

## 与 gdbus-learn 的关系

本项目不再依赖本地固定路径，而是通过 `CMake FetchContent` 直接拉取远端仓库：

- 依赖仓库：`https://github.com/SweerItTer/gdbus-learn.git`
- 当前默认版本：`94611e503f2cfcec045cf0bf2ba0fa62bebd16cc`

构建时会使用该仓库中的源码生成并链接这些目标：

- `training_common`
- `training_utils`
- `gdbus_generated`
- `training`
- `training_client_wrapper`
- `training_service_gui_demo`

GUI 自身通过继承 `training::client::TrainingClient` 与后端对接，直接复用：

- `GetTestInfo`
- `SetTestInfo`
- `SendFileByPath`
- `OnRemoteTestBoolChanged`
- `OnRemoteTestIntChanged`
- `OnRemoteTestDoubleChanged`
- `OnRemoteTestStringChanged`
- `OnRemoteTestInfoChanged`

## 同步逻辑说明

界面里只映射一个 `TestInfo`：

```cpp
using TestInfo = struct _testinfo {
    bool bool_param;
    int int_param;
    double double_param;
    std::string string_param;
};
```

同步规则如下：

1. `bool_param`
   映射到勾选框，控制输入框显示或隐藏。
2. `string_param`
   映射到输入框文本，任意窗口编辑后同步到另一个窗口。
3. `int_param + double_param`
   组成逻辑坐标，用于输入框拖动同步。
4. 拖动过程
   使用逻辑坐标预览，两个窗口按各自缩放比例显示相同相对位置。
5. 拖动结束
   将最终位置按整数对齐，写回 `int_param`，并将 `double_param` 清零。

## 文件发送

GUI 增加了文件发送操作区：

- `Select File`
  打开文件选择框，记录当前待发送文件路径。
- `Send File`
  调用 `TrainingClient::SendFileByPath(...)` 发送文件。
- 文件发送结果标签
  明确显示发送成功或失败，而不是只放在普通状态栏里。

## 依赖环境

需要以下基础工具和库：

- `build-essential`
- `cmake`
- `ninja-build`
- `pkg-config`
- `dbus-x11`
- `libglib2.0-dev`
- `libglib2.0-dev-bin`
- `libgl1-mesa-dev`
- `qt6-base-dev`
- `qt6-base-dev-tools`
- `qt6-tools-dev-tools`

Ubuntu / Debian 系可以安装：

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

或者直接运行：

```bash
./scripts/install_qt_ubuntu.sh
```

图形环境要求：

- Windows 11 + WSLg，或者
- 正常可用的 X11 / Wayland 显示环境

## 构建方式

### Debug / 默认构建

```bash
cmake -S . -B build-gui -G Ninja -DENABLE_QT_GUI=ON
cmake --build build-gui
```

### Release 构建

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_QT_GUI=ON
cmake --build build-release
```

### 指定 gdbus-learn 版本

如果要切换依赖仓库的提交或标签：

```bash
cmake -S . -B build-release -G Ninja \
  -DGDBUS_LEARN_GIT_TAG=<commit-or-tag> \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_QT_GUI=ON
```

### 纯逻辑测试

```bash
cmake -S . -B build-test -G Ninja -DENABLE_QT_GUI=OFF
cmake --build build-test
ctest --test-dir build-test --output-on-failure
```

## 一键运行

推荐直接使用：

```bash
./scripts/run_release.sh
```

它会自动完成：

1. `Release` 配置
2. `Release` 编译
3. 启动 `training_service_gui_demo`
4. 启动 `sync_visualizer`

## 手动运行

如果你已经完成编译，也可以手动启动：

```bash
dbus-run-session -- bash -lc './build-release/training_service_gui_demo & server_pid=$!; trap "kill $server_pid; wait $server_pid 2>/dev/null" EXIT; ./build-release/sync_visualizer'
```

## 使用方式

### 同步测试

1. 打开程序后会出现 `Window A` 和 `Window B`
2. 在任意窗口切换勾选框
3. 观察另一窗口输入框是否同步显示 / 隐藏
4. 在任意窗口输入文本
5. 观察另一窗口文本是否同步更新
6. 拖动输入框
7. 观察另一窗口位置是否同步变化
8. 松开鼠标后观察位置是否按整数对齐

### 文件发送测试

1. 点击 `Select File`
2. 选择一个本地文件
3. 点击 `Send File`
4. 观察界面中的文件发送结果标签

## 本地验证情况

当前已完成以下验证：

- `FetchContent` 远程拉取 `gdbus-learn` 构建通过
- Debug GUI 构建通过
- Release GUI 构建通过
- `ctest` 通过
- 一键启动脚本可用
- 离屏模式下 `service + GUI` 启动链路通过
