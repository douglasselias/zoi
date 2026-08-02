@echo off & cls

if not defined VCINSTALLDIR (
  call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)

rmdir /s /q build & mkdir build & echo * > build/.gitignore

cl /nologo /W4 /WX /wd4201 /wd4189 /wd4100 /wd4996 /O2 /Z7 /Fo.\build\ main.cpp /link /out:build\main.exe