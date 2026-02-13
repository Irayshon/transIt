@echo off
echo TransIt Uninstaller
echo ====================
echo.
echo This will remove TransIt settings from the Windows Registry.
echo.

reg delete "HKCU\Software\TransIt" /f >nul 2>&1
if %errorlevel%==0 (
    echo [OK] Registry settings removed.
) else (
    echo [OK] No registry settings found.
)

echo.
echo You can now delete the TransIt folder to complete uninstallation.
echo.
pause
