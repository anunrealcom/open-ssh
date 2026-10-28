@echo off
REM 一键生成发布目录 dist/（Release 构建 + 收集 Qt 依赖）
REM 需在 Qt 6.11.2 MinGW 64 工具链环境下运行（或直接在 Qt Creator 的 Release 构建后执行）
setlocal
set QT_DIR=C:\Qt\6.11.2\mingw_64
set MINGW_DIR=C:\Qt\Tools\mingw1310_64
set CMAKE_DIR=C:\Qt\Tools\CMake_64\bin
set PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%CMAKE_DIR%;%PATH%

cd /d %~dp0
if not exist build (
  cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
)
cmake --build build -j4
if errorlevel 1 exit /b 1

rmdir /s /q dist 2>nul
mkdir dist
copy build\xiaobaiOP.exe dist\
windeployqt --release --no-translations --no-opengl-sw dist\xiaobaiOP.exe
if exist README_防误报说明.txt copy README_防误报说明.txt dist\
echo.
echo [OK] 发布目录已生成：dist\
echo 然后用 Inno Setup 打开 xiaobaiOP_setup.iss 编译安装包。
endlocal
