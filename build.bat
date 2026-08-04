@echo off
setlocal enabledelayedexpansion

set "root_dir=%~dp0"

:: ---------------------------------------------------------------------------
:: Version management
:: ---------------------------------------------------------------------------
set "version_file=%root_dir%docs\version.txt"
if not exist "%version_file%" (
    echo Error: %version_file% not found >&2
    exit /b 1
)
set /p VERSION=<"%version_file%"
:: Trim whitespace
for /f "tokens=* delims= " %%a in ("%VERSION%") do set "VERSION=%%a"
echo Building Croissant %VERSION%

:: Stamp version into tauri.conf.json and Cargo.toml
powershell -Command "(Get-Content '%root_dir%main\src-tauri\tauri.conf.json') -replace '""version"":\s*""[^""]*""', '""version"": ""%VERSION%""' | Set-Content '%root_dir%main\src-tauri\tauri.conf.json'"
powershell -Command "(Get-Content '%root_dir%main\src-tauri\Cargo.toml') -replace '^version\s*=\s*""[^""]*""', 'version = ""%VERSION%""' | Set-Content '%root_dir%main\src-tauri\Cargo.toml'"
echo Stamped version %VERSION% into tauri.conf.json and Cargo.toml

:: ---------------------------------------------------------------------------
:: Pruning tables
:: ---------------------------------------------------------------------------
if not exist "%root_dir%main\src-tauri\resources\pruning-tables" (
    if exist "%root_dir%legacy\build\Desktop-Debug\pruning-tables" (
        mkdir "%root_dir%main\src-tauri\resources\pruning-tables"
        copy "%root_dir%legacy\build\Desktop-Debug\pruning-tables\*.dat" "%root_dir%main\src-tauri\resources\pruning-tables\"
    )
)
echo Prepared embedded solver pruning tables

:: ---------------------------------------------------------------------------
:: Build
:: ---------------------------------------------------------------------------
echo Generating desktop icons...
cd /d "%root_dir%main"
call npx tauri icon "%root_dir%icons\icon-pc.png"
copy /y "%root_dir%main\src-tauri\icons\icon.ico" "%root_dir%legacy\res\icon.ico" >nul

call npm ci
call npx tauri build

:: ---------------------------------------------------------------------------
:: Rename artifacts
:: ---------------------------------------------------------------------------
set "output_dir=%root_dir%output"
if not exist "%output_dir%" mkdir "%output_dir%"

:: Copy .exe (NSIS installer) and .msi if present
for %%E in (exe msi) do (
    for /r "%root_dir%main\src-tauri\target\release\bundle" %%F in (*.%%E) do (
        echo   -^> %output_dir%\%%~nxF
        copy "%%F" "%output_dir%\" >nul
    )
)

echo.
echo Build complete. Version: %VERSION%
echo Artifacts in %output_dir%:
dir /b "%output_dir%"
