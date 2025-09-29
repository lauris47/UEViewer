@echo off
set PATH=C:\Users\lauri\Documents\Github\UEViewer\BuildTools\bin;%PATH%
bash build.sh --64


copy /Y .\umodel_64.exe "C:\p4v\laurynas_RocketLeague\Plugins\ProjectPlugins\MapImporter\Source\MapImporter\UModel\umodel_64.exe"