@ECHO off

SET ROOT=%~dp0
SET BUILD_DIR=%ROOT%\build

SET WARNINGS=/W4 /wd4996 /wd4577
SET FLAGS=/Zi

IF NOT [%1]==[] (
	IF %1=="debug" (SET FLAGS=%FLAGS% /Od)
	IF %1=="release" (SET FLAGS=%FLAGS% /O3)
)

SET TOOLS_PREPROCESSOR_CPP=%ROOT%\tools\preprocessor.cpp

pushd %BUILD_DIR%\tools
cl %WARNINGS% %FLAGS% %TOOLS_PREPROCESSOR_CPP% 
POPD

PUSHD %BUILD_DIR%
msbuild /property:GenerateFullPaths=true leary.vcxproj
POPD

