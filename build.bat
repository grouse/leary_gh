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
if not exist %BUILD_DIR%\data mkdir %BUILD_DIR%\data
if not exist %BUILD_DIR%\data\shaders mkdir %BUILD_DIR%\data\shaders

SET INCLUDE_DIR=%INCLUDE_DIR% -I%VULKAN_LOCATION%\Include

SET LIBS=User32.lib Shlwapi.lib Shell32.lib %VULKAN_LOCATION%\Bin\vulkan-1.lib

PUSHD %BUILD_DIR%
REM TODO(jesper): move this into a separate build script; we won't need to and
REM don't want to rebuild all the assets every build, it'll become real slow
REM real fast
glslangValidator -V %ROOT%\src\render\shaders\generic.frag -o %BUILD_DIR%\data\shaders\generic.frag.spv
glslangValidator -V %ROOT%\src\render\shaders\generic.vert -o %BUILD_DIR%\data\shaders\generic.vert.spv

cl %FLAGS% %UNOPTIMIZED% %INCLUDE_DIR% /Feleary.exe %ROOT%\src\platform/win32_main.cpp /link %LIBS% /SUBSYSTEM:WINDOWS
POPD
