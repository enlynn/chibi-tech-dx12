@echo off

:: Project directory
SET HOST_DIR=%~dp0
SET HOST_DIR=%HOST_DIR:~0,-1%
SET HOST_DIR=%HOST_DIR%\..

:: Core Shaders

SET IN=%HOST_DIR%\content\shaders
SET OUT=%HOST_DIR%\content\shaders\out
IF NOT EXIST %OUT% mkdir %OUT%

SET FLAGS=
SET MODE=release
IF %MODE% equ debug (
	SET FLAGS=%FLAGS% -Qembed_debug
) else (
	SET FLAGS=%FLAGS% -O3
)

SET DXC=%HOST_DIR%\vendor\dxc\bin\x64\dxc.exe
SET DXIL=%HOST_DIR%\vendor\dxc\bin\x64\dxil.dll

del %OUT%\*.cso

%DXC% %FLAGS% -T vs_6_5 -Fo%OUT%\TestTriangle.Vtx.cso             %IN%\TestTriangle.Vtx.hlsl
%DXC% %flags% -T ps_6_5 -Fo%OUT%\TestTriangle.Pxl.cso             %IN%\TestTriangle.Pxl.hlsl