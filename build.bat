@echo off
cls

if not defined VCINSTALLDIR (
  call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)

rmdir /s /q build
mkdir build
echo * > build/.gitignore

set shader_flags=/nologo /WX /Ges /Zpr

:: fxc src/main.hlsl %shader_flags% /T vs_5_0 /E vs_main /Fo NUL
:: fxc src/main.hlsl %shader_flags% /T ps_5_0 /E ps_main /Fo NUL

cl /nologo /W4 /WX /wd4201 /wd4189 /wd4100 /wd4996 /Z7 /Fo.\build\ main.cpp /link /out:build\main.exe