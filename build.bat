@echo off
REM =====================================================================
REM  xiaobaiOP 一键编译脚本（Windows + Qt6 MinGW）
REM  用法：
REM    build.bat          正常配置并编译
REM    build.bat clean     清理 build 目录后重新编译
REM =====================================================================
setlocal enabledelayedexpansion

REM ---- 1. 工具链路径（如安装位置不同，请修改此处） ----
set "QT_KIT=C:/Qt/6.11.2/mingw_64"
set "MINGW=C:/Qt/Tools/mingw1310_64"
set "CMAKE=C:/Qt/Tools/CMake_64/bin"

REM ---- 2. 把 Qt 工具链放到 PATH 最前 ----
set "PATH=%QT_KIT%\bin;%MINGW%\bin;%CMAKE%\bin;%PATH%"

REM ---- 3. 临时禁用 Git 的 sh.exe ----
REM  Git 自带 MSYS 的 sh.exe 会被 CMake 误用，触发 "/c/..." 路径转换，
REM  导致 find_package(Qt6) 静默失败。编译期间临时改名，结束后还原。
set "RESTORE_SH="
if exist "C:\Program Files\Git\usr\bin\sh.exe" (
    rename "C:\Program Files\Git\usr\bin\sh.exe" sh.exe.bak >nul 2>&1
    if exist "C:\Program Files\Git\usr\bin\sh.exe.bak" (
        set "RESTORE_SH=C:\Program Files\Git\usr\bin\sh.exe"
    )
)
if exist "D:\Program Files\Git\usr\bin\sh.exe" (
    rename "D:\Program Files\Git\usr\bin\sh.exe" sh.exe.bak >nul 2>&1
    if exist "D:\Program Files\Git\usr\bin\sh.exe.bak" (
        set "RESTORE_SH=D:\Program Files\Git\usr\bin\sh.exe"
    )
)

REM ---- 4. 清理 ----
if "%1"=="clean" (
    echo [清理] 删除 build 目录...
    if exist build rmdir /s /q build
)

REM ---- 5. 配置 + 编译 ----
echo [配置] CMake ...
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%QT_KIT%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :fail

echo [编译] mingw32-make ...
cmake --build build -j
if errorlevel 1 goto :fail

echo.
echo ================= 编译成功 =================
echo 可执行文件: %CD%\build\xiaobaiOP.exe
echo ============================================
goto :done

:fail
echo.
echo [错误] 编译失败，请根据上方日志排查。
goto :done

:done
REM ---- 6. 还原 sh.exe ----
if defined RESTORE_SH (
    rename "%RESTORE_SH%.bak" sh.exe >nul 2>&1
)
endlocal
