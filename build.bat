@echo off
cls

if not defined VCINSTALLDIR (
  call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)

rmdir /s /q build
mkdir build
echo * > build/.gitignore

cl /nologo /W4 /WX /Z7 /Fo.\build\ main.cpp /link /out:build\main.exe