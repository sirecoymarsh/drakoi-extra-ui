@echo off
setlocal
set "ROOT=%~dp0"
if not exist "%ROOT%temp" mkdir "%ROOT%temp"
if not exist "%ROOT%build" mkdir "%ROOT%build"
if not exist "%ROOT%rendered" mkdir "%ROOT%rendered"
set "TEMP=%ROOT%temp"
set "TMP=%TEMP%"
call "%ROOT%tools\setup-msvc.bat"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /TC /utf-8 /W4 /WX /wd4505 /O2 /I"%ROOT%source" /Fo"%ROOT%build\native_update_review.obj" /Fe"%ROOT%build\native_extra_review.exe" "%ROOT%native_update_review.c" /link user32.lib gdi32.lib winmm.lib msimg32.lib ole32.lib windowscodecs.lib /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'"
if errorlevel 1 exit /b %errorlevel%
"%ROOT%build\native_extra_review.exe" "%ROOT%." %*
exit /b %errorlevel%
