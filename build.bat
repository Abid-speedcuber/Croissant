@echo off
setlocal enabledelayedexpansion

set "root_dir=%~dp0"

if not exist "%root_dir%main\src-tauri\resources\pruning-tables" (
    if exist "%root_dir%legacy\build\Desktop-Debug\pruning-tables" (
        mkdir "%root_dir%main\src-tauri\resources\pruning-tables"
        copy "%root_dir%legacy\build\Desktop-Debug\pruning-tables\*.dat" "%root_dir%main\src-tauri\resources\pruning-tables\"
    )
)
echo Prepared embedded solver pruning tables

echo Generating desktop icons...
cd /d "%root_dir%main"
call npx tauri icon "%root_dir%icons\icon-pc.png"
copy /y "%root_dir%main\src-tauri\icons\icon.ico" "%root_dir%legacy\res\icon.ico" >nul

call npm ci
call npx tauri build
