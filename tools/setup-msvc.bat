@echo off
if /I "%VSCMD_ARG_TGT_ARCH%"=="x86" (
  where cl.exe >nul 2>nul
  if not errorlevel 1 exit /b 0
)
set "DRAKOI_VS_ROOT="
set "DRAKOI_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%DRAKOI_VSWHERE%" (
  for /f "usebackq delims=" %%I in (`"%DRAKOI_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "DRAKOI_VS_ROOT=%%I"
)
if not defined DRAKOI_VS_ROOT if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" set "DRAKOI_VS_ROOT=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
if not defined DRAKOI_VS_ROOT (
  echo MSVC x86 tools were not found. Install Visual Studio C++ Build Tools and a Windows SDK.
  exit /b 1
)
call "%DRAKOI_VS_ROOT%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
exit /b %errorlevel%
