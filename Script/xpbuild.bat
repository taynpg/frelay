@echo on

set QT_563_ROOT=C:\Qt\Qt5.6.3
set QT_DIRS="%QT_563_ROOT%\5.6.3\mingw49_32"
set QT_GCC_ROOT="%QT_563_ROOT%\Tools\mingw49_32\bin"

set MINGWDLL1="%QT_563_ROOT%\Tools\mingw492_32\bin\libgcc_s_dw2-1.dll"
set MINGWDLL2="%QT_563_ROOT%\Tools\mingw492_32\bin\libstdc++-6.dll"
set MINGWDLL3="%QT_563_ROOT%\Tools\mingw492_32\bin\libwinpthread-1.dll"
set QTDLL1="%QT_563_ROOT%\5.6.3\mingw49_32\bin\Qt5Core.dll"
set QTDLL2="%QT_563_ROOT%\5.6.3\mingw49_32\bin\Qt5Gui.dll"
set QTDLL3="%QT_563_ROOT%\5.6.3\mingw49_32\bin\Qt5Widgets.dll"
set QTDLL4="%QT_563_ROOT%\5.6.3\mingw49_32\bin\Qt5Network.dll"
set QTDLL5="%QT_563_ROOT%\5.6.3\mingw49_32\plugins\platforms\qwindows.dll"

set PAHT=%PATH%;%QT_GCC_ROOT%;

cmake -B"%~dp0..\xpbuild" -S"%~dp0.." -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_DIRS% -DQT_DEFAULT_MAJOR_VERSION=5 -DXP_PLATFORM_SUPPORT=ON
cmake --build "%~dp0..\xpbuild" --config Release

if %errorlevel% equ 0 (
    if not exist "%~dp0..\xpbuild\bin\Release\platforms" (
        mkdir "%~dp0..\xpbuild\bin\Release\platforms"
    )
    xcopy /Y %MINGWDLL1% "%~dp0..\xpbuild\bin\Release"
    xcopy /Y %MINGWDLL2% "%~dp0..\xpbuild\bin\Release"
    xcopy /Y %MINGWDLL3% "%~dp0..\xpbuild\bin\Release"
    xcopy /Y %QTDLL1% "%~dp0..\xpbuild\bin\Release"
    xcopy /Y %QTDLL2% "%~dp0..\xpbuild\bin\Release"
    xcopy /Y %QTDLL3% "%~dp0..\xpbuild\bin\Release"
    xcopy /Y %QTDLL4% "%~dp0..\xpbuild\bin\Release"
    xcopy /Y %QTDLL5% "%~dp0..\xpbuild\bin\Release\platforms"
    del /f /q "%~dp0..\xpbuild\bin\Release\frelayBaseTest.exe"
    del /f /q "%~dp0..\xpbuild\bin\Release\frelayTest.exe"
) 

pause