# 小白运维 xiaobaiOP（Qt/C++ 单机桌面版）

基于 Qt6 Widgets + CMake 重写的**单机桌面运维工具**，对标 MobaXterm / XShell。
与 `xiaobai-ops`（Go+Vue Web 版）相互独立，本工程不依赖任何后端服务。

## 功能

| 模块 | 说明 |
| --- | --- |
| SSH 终端 | 类 XShell 的交互终端，支持 ANSI 颜色、方向键、Ctrl 组合键；基于系统 OpenSSH（`ssh -tt`） |
| 会话管理 | 会话树（左侧），保存主机/用户/密码/密钥/端口/操作系统；支持新增、编辑、删除、双击连接 |
| 快速连接 | 不保存，临时连一下 |
| 文件传输 | 双栏（本地/远程）文件管理器；远程列目录走 `ssh ls`，下载/上传走 `scp`（SFTP over SSH） |
| 远程桌面 | Windows 目标调用系统 `mstsc`（RDP）；Linux 目标按配置调用 VNC 查看器 |

> 已**取消**登录、MySQL、Redis 等第三方服务端依赖——纯单机工具，会话仅存于本机 JSON 文件。

## 技术栈与依赖

- Qt 6（Widgets）
- CMake ≥ 3.16（开启 `AUTOMOC`）
- C++17
- **零额外第三方库**：SSH / SCP / RDP 全部复用系统自带能力（Windows 10+ 自带 OpenSSH 客户端）

## 编译（Windows / MinGW）

需要 Qt6（mingw_64 套件）与 MinGW 编译器。示例（路径按本机调整）：

```bat
set PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;%PATH%
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.11.2\mingw_64 -S . -B build
cmake --build build -j4
```

> 注意：CMake 是原生 Windows 程序，前缀路径必须用 Windows 风格 `C:/Qt/...`，
> 不能用 MSYS 风格的 `/c/Qt/...`；且配置时 PATH 中不要存在 `sh.exe`（MSYS），否则路径转换会出错。

产物：`build/xiaobaiOP.exe`。

## 使用

1. 工具栏「新建会话」填入主机/用户/密码（或私钥），保存并连接；
2. 终端标签页即可输入命令，支持 vim / top 等交互程序；
3. 工具栏「文件传输」打开右侧面板，浏览、上传、下载文件；
4. 选中会话后点「远程桌面」：Windows 走 mstsc，Linux 走配置的 VNC 命令。

## 目录结构

```
xiaobaiOP/
  CMakeLists.txt
  src/
    main.cpp
    session.h/.cpp          会话模型 + JSON 存储
    sshsession.h/.cpp       QProcess + OpenSSH 交互会话
    terminalwidget.h/.cpp   ANSI 终端视图 + 键盘映射
    filetransfer.h/.cpp     scp/ssh 文件传输
    sessiondialog.h/.cpp    新建/编辑/快速连接对话框
    filewidget.h/.cpp       文件传输面板
    mainwindow.h/.cpp       主窗口（会话树 + 标签页 + 文件 Dock）
```
