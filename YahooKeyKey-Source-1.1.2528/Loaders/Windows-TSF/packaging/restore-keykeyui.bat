@echo off
setlocal
rem Puts the original KeyKeyUI.dll back.
rem Right-click -> Run as administrator.

net session >nul 2>&1
if errorlevel 1 (
    echo Please right-click this file and choose "Run as administrator".
    pause
    exit /b 1
)

set "KEYDIR="
for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\Yahoo\KeyKey" /v ServerLocation 2^>nul ^| findstr /i "ServerLocation"') do set "KEYDIR=%%B"

if not defined KEYDIR (
    echo KeyKey's install folder was not found.
    pause
    exit /b 1
)

if not exist "%KEYDIR%\KeyKeyUI.dll.orig" (
    echo No backup found - nothing to restore.
    pause
    exit /b 1
)

taskkill /f /im KeyKeyServer.exe >nul 2>&1
timeout /t 2 >nul

copy /y "%KEYDIR%\KeyKeyUI.dll.orig" "%KEYDIR%\KeyKeyUI.dll" >nul
if errorlevel 1 (
    echo Restore failed - is KeyKey still running?
    pause
    exit /b 1
)

echo Restored the original KeyKeyUI.dll.
pause
