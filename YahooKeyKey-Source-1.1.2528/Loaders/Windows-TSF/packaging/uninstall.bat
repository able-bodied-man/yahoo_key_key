@echo off
setlocal
rem Removes the KeyKey TSF text service. Right-click -> Run as administrator.

net session >nul 2>&1
if errorlevel 1 (
    echo Please right-click this file and choose "Run as administrator".
    pause
    exit /b 1
)

if exist "%SystemRoot%\System32\regsvr32.exe" "%SystemRoot%\System32\regsvr32.exe" /u /s "%~dp0KeyKeyTSF.dll"
if exist "%SystemRoot%\SysWOW64\regsvr32.exe" "%SystemRoot%\SysWOW64\regsvr32.exe" /u /s "%~dp0KeyKeyTSF.dll"

echo Unregistered. If the input method still shows up, remove it in
echo Settings ^> Time ^& language ^> Language ^& region ^> keyboard list.
pause
