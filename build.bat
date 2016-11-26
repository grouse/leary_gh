@ECHO off

SET ROOT=%~dp0
SET BUILD_DIR=%ROOT%\build

SET INCLUDE_DIR=-I%ROOT%\src
SET WARNINGS=/W4 /wd4996 /wd4577 /wd4065
SET DEFINITIONS=-DWIN32_LEAN_AND_MEAN -DNOMINMAX

SET FLAGS=%WARNINGS% %DEFINITIONS%
SET FLAGS=%FLAGS% /Zi /EHsc

SET OPTIMIZED=/O2
SET UNOPTIMIZED=/Od

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %BUILD_DIR%\tools mkdir %BUILD_DIR%\tools
if not exist %BUILD_DIR%\debug mkdir %BUILD_DIR%\debug
if not exist %BUILD_DIR%\release mkdir %BUILD_DIR%\release

PUSHD %BUILD_DIR%\tools
cl %FLAGS% %OPTIMIZED% %ROOT%\tools\preprocessor.cpp /link /SUBSYSTEM:CONSOLE
REM cl %FLAGS% %UNOPTIMIZED% %ROOT%\tools\preprocessor.cpp /link /SUBSYSTEM:CONSOLE
POPD

SET INCLUDE_DIR=%INCLUDE_DIR% -I%VULKAN_LOCATION%\Include

SET LIBS=User32.lib Shlwapi.lib Shell32.lib
SET LIBS=%LIBS% %VULKAN_LOCATION%\Bin\vulkan-1.lib

PUSHD %BUILD_DIR%\%1
REM cl %FLAGS% &OPTIMIZED% %INCLUDE_DIR% %ROOT%\src\platform/win32_main.cpp /link %LIBS% /SUBSYSTEM:WINDOWS
cl %FLAGS% %UNOPTIMIZED% %INCLUDE_DIR% %ROOT%\src\platform/win32_main.cpp /link %LIBS% /SUBSYSTEM:WINDOWS
POPD 
