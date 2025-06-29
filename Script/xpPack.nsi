!define APP_NAME "frelay"
!define APP_VERSION "0.2.0.0"
!define COMPANY_NAME "Taynpg"
!define INSTALL_DIR "$PROFILE\frelay"
!define SOURCE_DIR "..\xpbuild\bin\Release"

Name "${APP_NAME}"
OutFile "${APP_NAME}Setup_${APP_VERSION}.exe"
InstallDir "${INSTALL_DIR}"
InstallDirRegKey HKCU "Software\${COMPANY_NAME}\${APP_NAME}" "InstallDir"
RequestExecutionLevel user

!include "MUI2.nsh"
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "Main Program" SecMain
    SetOutPath "$INSTDIR"
    File /r "${SOURCE_DIR}\*.*"
    CreateShortCut "$DESKTOP\${APP_NAME} Console.lnk" "$INSTDIR\frelayConsole.exe"
    CreateShortCut "$DESKTOP\${APP_NAME} GUI.lnk" "$INSTDIR\frelayGUI.exe"
    CreateShortCut "$DESKTOP\${APP_NAME} Server.lnk" "$INSTDIR\frelayServer.exe"
    WriteRegStr HKCU "Software\${COMPANY_NAME}\${APP_NAME}" "InstallDir" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${COMPANY_NAME}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "$DESKTOP\${APP_NAME} Console.lnk"
    Delete "$DESKTOP\${APP_NAME} GUI.lnk"
    Delete "$DESKTOP\${APP_NAME} Server.lnk"
    RMDir /r "$INSTDIR"
    DeleteRegKey HKCU "Software\${COMPANY_NAME}\${APP_NAME}"
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd

VIProductVersion "${APP_VERSION}.0"
VIAddVersionKey "ProductName" "${APP_NAME}"
VIAddVersionKey "FileDescription" "${APP_NAME} Installer"
VIAddVersionKey "FileVersion" "${APP_VERSION}"
VIAddVersionKey "CompanyName" "${COMPANY_NAME}"
VIAddVersionKey "LegalCopyright" "? ${COMPANY_NAME}"