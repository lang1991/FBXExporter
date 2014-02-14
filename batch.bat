@ECHO OFF
set "str=%~dp0%FBXExporter\testModels\*.fbx"
set "str1=%~dp0%FBXExporter\exportedModels\"
setlocal ENABLEDELAYEDEXPANSION
for %%i in ("%str%") do (
	echo %%i
	START "" ".\Release\FBXExporter.exe" %%i %str1%
)
pause