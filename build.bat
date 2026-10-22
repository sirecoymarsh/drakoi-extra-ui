@echo off
setlocal EnableExtensions
set "ROOT=%~dp0"
if not exist "%ROOT%temp" mkdir "%ROOT%temp"
if not exist "%ROOT%build" mkdir "%ROOT%build"
if not exist "%ROOT%rendered" mkdir "%ROOT%rendered"
set "TEMP=%ROOT%temp"
set "TMP=%ROOT%temp"
call "%ROOT%tools\setup-msvc.bat"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /TC /utf-8 /W4 /WX /wd4152 /O2 /GS /guard:cf /I"%ROOT%source" /c /Fo"%ROOT%build\vocal_probe.obj" "%ROOT%source\vocal_probe.c"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /LD /Fe:"%ROOT%build\ko.dll" "%ROOT%build\ko_hook_r2e.obj" "%ROOT%build\ko_r2c_ui.obj" "%ROOT%build\vocal_probe.obj" user32.lib gdi32.lib msimg32.lib winmm.lib ole32.lib windowscodecs.lib /link /Brepro /DYNAMICBASE /NXCOMPAT /GUARD:CF /MACHINE:X86 /MAP:"%ROOT%build\ko.map" /IMPLIB:"%ROOT%build\ko.lib"
exit /b %errorlevel%

