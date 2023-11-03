@echo off

:: Project directory
SET HOST_DIR=%~dp0
SET HOST_DIR=%HOST_DIR:~0,-1%

::-----------------------------------------------------------------------------------------------------------------------
:: Setup build tools

:: Ideally, we only call this once, but for some silly reason, calling setup should initialize all of the env variables
:: but it doesn't seem to persist across compilation calls. I am suspicious it has to do with my PATH variable being too
:: long, but I am not about to go deleting my path variables. It seems to be fixed by calling setup for every compilation.

cl >nul 2>nul
if %errorlevel% neq 9009 goto :build
echo Running VS build tool setup.
echo Initializing MS build tools...
call %HOST_DIR%\scripts\setup_cl.bat
cl >nul 2>nul
if %errorlevel% neq 9009 goto :build
echo Unable to find build tools! Make sure that you have Microsoft Visual Studio 10 or above installed!
exit /b 1
::-----------------------------------------------------------------------------------------------------------------------

:build

:: Configuration

SET BUILD_PATH="%HOST_DIR%/out/build/x64-debug"
IF /i $%1 equ $Release (set BUILD_PATH="%HOST_DIR%/out/build/x64-release")

SET BUILD_PROGRAM="ninja.exe"

SET CMAKE_PRESET="x64-debug"
IF /i $%1 equ $Release (set CMAKE_PRESET="x64-release")

set CMAKE_CMD=cmake.exe

IF NOT EXIST %BUILD_PATH%   mkdir %BUILD_PATH%

pushd %BUILD_PATH%
	del *.pdb > NUL 2> NUL
	:: Run cmake in case any new files have been added
	%CMAKE_CMD% -S %HOST_DIR% --preset=%CMAKE_PRESET%
	%BUILD_PROGRAM% -f%BUILD_PATH%/build.ninja
popd

echo Build complete!
exit /b 0

:fail
echo Build failed!
exit /b %errorlevel%