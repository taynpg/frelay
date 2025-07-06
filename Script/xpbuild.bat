@echo on

set QT_DIR=C:\Qt\Qt5.7.1\5.7\mingw53_32
set COMPILE_DIR=C:\Qt\Qt5.7.1\Tools\mingw530_32

set PATH=%PATH%;%COMPILE_DIR\bin%

xmake f -p mingw --sdk=C:\Qt\Qt5.7.1\Tools\mingw530_32 -a i386 -m release --qt=%QT_DIR% --gui=y --qt5=y --xp=y -o build-xp -v
xmake
set outDir=%~dp0..\build-xp\mingw\i386\release\
if %errorlevel% equ 0 (
    del /q "%outDir%\*.a" 2>nul
    for /f "delims=" %%f in ('dir /b /a-d "%outDir%\*Test*" 2^>nul') do (
            del /q "%outDir%\%%f"
        )
    %QT_DIR%\bin\windeployqt.exe %outDir%frelayGUI.exe
)