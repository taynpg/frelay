@echo off
cd ..
xmake f -p windows -a x64 -m release --qt=C:\Qt\6.8.3 --gui=y -o build-qt6 -v
xmake -r
set outDir=%~dp0..\build-qt6\windows\x64\release\
if %errorlevel% equ 0 (
	del /q "%outDir%\*.lib" 2>nul
	for /f "delims=" %%f in ('dir /b /a-d "%outDir%\*Test*" 2^>nul') do (
        	del /q "%outDir%\%%f"
        )
	C:\Qt\6.8.3\bin\windeployqt.exe %outDir%frelayGUI.exe
)
pause