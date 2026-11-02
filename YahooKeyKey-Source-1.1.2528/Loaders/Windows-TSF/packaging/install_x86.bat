@echo off
setlocal
rem Registers the 32-bit KeyKeyTSF.dll. Right-click -> Run as administrator.

net session >nul 2>&1
if errorlevel 1 (
    echo Please right-click this file and choose "Run as administrator".
    pause
    exit /b 1
)

if not exist "%~dp0KeyKeyTSF.dll" (
    echo KeyKeyTSF.dll is missing next to this script.
    pause
    exit /b 1
)

"%SystemRoot%\SysWOW64\regsvr32.exe" /s "%~dp0KeyKeyTSF.dll"
if errorlevel 1 (
    echo Registration failed.
    pause
    exit /b 1
)

echo.
echo Registered "Yahoo! KeyKey (TSF)" (32-bit).
echo Add it under: Settings ^> Time ^& language ^> Language ^& region
echo                ^> Chinese (Traditional, Taiwan) ^> Add a keyboard
echo then switch input methods with Win+Space.
echo.
pause
