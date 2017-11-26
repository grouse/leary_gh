@ECHO off

SET ROOT=%~dp0
SET BUILD_DIR=%ROOT%\build

SET INCLUDE_DIR=-I%ROOT%\src -I%ROOT%
SET WARNINGS=/W4 /wd4996 /wd4577 /wd4065 /wd4800 /wd4201
SET DEFINITIONS=-DWIN32_LEAN_AND_MEAN -DNOMINMAX

SET FLAGS=%WARNINGS% %DEFINITIONS%
SET FLAGS=%FLAGS% /Zi /EHsc /permissive-

SET OPTIMIZED=/O2
SET UNOPTIMIZED=/Od

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %BUILD_DIR%\data mkdir %BUILD_DIR%\data
if not exist %BUILD_DIR%\data\shaders mkdir %BUILD_DIR%\data\shaders

echo "-----------"
echo %VULKAN_LOCATION%

SET INCLUDE_DIR=%INCLUDE_DIR% -I%VULKAN_LOCATION%\Include

SET LIBS=User32.lib Shlwapi.lib Shell32.lib %VULKAN_LOCATION%\Lib\vulkan-1.lib
echo %LIBS%

PUSHD %BUILD_DIR%
REM TODO(jesper): move this into a separate build script; we won't need to and
REM don't want to rebuild all the assets every build, it'll become real slow
REM real fast
glslangValidator -V %ROOT%\src\shaders\generic.glsl -DVERTEX_SHADER -S vert -g -o %BUILD_DIR%\data\shaders\generic.vert.spv
glslangValidator -V %ROOT%\src\shaders\generic.glsl -DFRAGMENT_SHADER -S frag -g -o %BUILD_DIR%\data\shaders\generic.frag.spv
glslangValidator -V %ROOT%\src\shaders\font.glsl -DVERTEX_SHADER -S vert -g -o %BUILD_DIR%\data\shaders\font.vert.spv
glslangValidator -V %ROOT%\src\shaders\font.glsl -DFRAGMENT_SHADER -S frag -g -o %BUILD_DIR%\data\shaders\font.frag.spv
glslangValidator -V %ROOT%\src\shaders\basic2d.glsl -DVERTEX_SHADER -S vert -g -o %BUILD_DIR%\data\shaders\basic2d.vert.spv
glslangValidator -V %ROOT%\src\shaders\basic2d.glsl -DFRAGMENT_SHADER -S frag -g -o %BUILD_DIR%\data\shaders\basic2d.frag.spv
glslangValidator -V %ROOT%\src\shaders\mesh.glsl -DVERTEX_SHADER -S vert -g -o %BUILD_DIR%\data\shaders\mesh.vert.spv
glslangValidator -V %ROOT%\src\shaders\mesh.glsl -DFRAGMENT_SHADER -S frag -g -o %BUILD_DIR%\data\shaders\mesh.frag.spv
glslangValidator -V %ROOT%\src\shaders\terrain.glsl -DVERTEX_SHADER -S vert -g -o %BUILD_DIR%\data\shaders\terrain.vert.spv
glslangValidator -V %ROOT%\src\shaders\terrain.glsl -DFRAGMENT_SHADER -S frag -g -o %BUILD_DIR%\data\shaders\terrain.frag.spv

REM cl %FLAGS% %UNOPTIMIZED% %INCLUDE_DIR% %ROOT%\src\platform\win32_leary.cpp /link %LIBS% /DLL /OUT:game.dll
cl %FLAGS% %UNOPTIMIZED% %INCLUDE_DIR% /Feleary.exe %ROOT%\src\platform/win32_main.cpp /SUBSYSTEM:WINDOWS /link %LIBS%
REM cl %FLAGS% %OPTIMIZED% %INCLUDE_DIR% /Febenchmark.exe %ROOT%\benchmarks\main.cpp /SUBSYSTEM:CONSOLE /link %LIBS%
POPD
