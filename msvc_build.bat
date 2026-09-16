@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SRC=%~1"
if "%SRC%"=="" set "SRC=%~dp0Source.cpp"

if not exist "%SRC%" (
  echo [ERROR] Source not found: %SRC%
  exit /b 1
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VCVARS="

if exist "%VSWHERE%" (
  for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
  )
)

if "%VCVARS%"=="" if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
  set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)
if "%VCVARS%"=="" if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
  set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
)
if "%VCVARS%"=="" if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
  set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)

if "%VCVARS%"=="" (
  echo [ERROR] Visual Studio C++ build tools not found.
  echo Install VS 2022 with "Desktop development with C++".
  exit /b 1
)

call "%VCVARS%" >nul
if errorlevel 1 (
  echo [ERROR] Failed to load vcvars64.bat
  exit /b 1
)

for %%F in ("%SRC%") do (
  set "SRCDIR=%%~dpF"
  set "SRCBASE=%%~nF"
)

cd /d "%SRCDIR%"
set "EXE=%SRCDIR%%SRCBASE%.exe"
set "OBJ=%SRCDIR%%SRCBASE%.obj"

if exist "%EXE%" del /f /q "%EXE%" 2>nul
if exist "%OBJ%" del /f /q "%OBJ%" 2>nul

echo Building "%SRC%" ...
cl /nologo /EHsc /std:c++17 /O2 /Fe"%EXE%" /Fo"%OBJ%" "%SRC%" user32.lib
set "ERR=%ERRORLEVEL%"

if exist "%OBJ%" del /f /q "%OBJ%" 2>nul

if not "%ERR%"=="0" (
  echo [ERROR] Compile failed. EXITCODE=%ERR%
  exit /b %ERR%
)

if not exist "%EXE%" (
  echo [ERROR] EXE was not produced.
  exit /b 1
)

echo [OK] Built: %EXE%
exit /b 0
