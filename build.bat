@echo off

if not exist build mkdir build

set PROJ_DIR=%cd%
set FLAGS= -std:c11 -nologo -EHsc -Zi -Od -W3


rem START BUILD
rem ============================================================
set path=%PROJECT_DIR%\build;%path%

rem nasm -h
pushd build
nasm -o .\decodeme  %PROJ_DIR%\src\decodeme.asm 
cl %FLAGS% %PROJ_DIR%\src\decoder8086.c
popd

pause
