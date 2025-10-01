@echo off
setlocal

set "PATH=C:\Users\lauri\Documents\Github\UEViewer\BuildTools\bin;%PATH%"

rem Source and destination
set "SRC=.\umodel_64.exe"
set "DST=C:\p4v\laurynas_RocketLeague\Plugins\ProjectPlugins\MapImporter\Source\MapImporter\UModel\umodel_64.exe"

rem Build first (bash returns nonzero on failure)
bash build.sh --64
if errorlevel 1 (
    echo Build failed, aborting.
    exit /b 1
)

rem If destination exists, delete it and verify deletion succeeded
if exist "%DST%" (
    echo Destination exists, attempting to delete "%DST%".
    del /F /Q "%DST%"
    if exist "%DST%" (
        echo ERROR: failed to delete "%DST%".
        exit /b 1
    )
    echo Deleted existing file.
)

rem Copy and verify
copy /Y "%SRC%" "%DST%" >nul
if errorlevel 1 (
    echo ERROR: copy failed.
    exit /b 1
)

if not exist "%DST%" (
    echo ERROR: destination not found after copy.
    exit /b 1
)

echo Copy succeeded.
endlocal
exit /b 0