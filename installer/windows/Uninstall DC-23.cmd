@echo off
setlocal EnableExtensions
rem DC-23 ONLY: resolve the exact Inno Setup uninstaller from its registry AppId.
rem Other VST3 plugins and uninstallers in the shared VST3 directory are not touched.
set "REGKEY=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\DC23_VST3_DioalexMusicTools_is1"
set "UNINSTALL_CMD="
for /f "tokens=2,*" %%A in ('reg query "%REGKEY%" /v UninstallString 2^>nul ^| findstr /i "UninstallString"') do set "UNINSTALL_CMD=%%B"
if not defined UNINSTALL_CMD (
    echo DC-23 v1.0 uninstaller was not found in the Windows installation registry.
    echo Open Settings ^> Apps ^> Installed apps and uninstall DC-23 from there.
    pause
    exit /b 1
)
echo Starting the registered DC-23 uninstaller...
start "" %UNINSTALL_CMD%
