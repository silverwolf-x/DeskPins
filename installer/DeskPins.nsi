;; DeskPins.nsi (UTF-8 with BOM)
;;
;; Build (done by CI):
;;   makensis /DVERSION=1.33.0 /DBINDIR=<dir with DeskPins.exe> /DOUTFILE=<setup.exe> installer\DeskPins.nsi

Unicode true
SetCompressor /SOLID lzma
ManifestDPIAware true
RequestExecutionLevel admin

!include "LogicLib.nsh"
!include "x64.nsh"
!include "WinMessages.nsh"

!ifndef VERSION
    !error "VERSION not defined"
!endif
!ifndef BINDIR
    !error "BINDIR not defined"
!endif
!ifndef OUTFILE
    !define OUTFILE "DeskPins-${VERSION}-setup.exe"
!endif

!define APP_KEY     "Software\Elias Fotinis\DeskPins"
!define UNINST_PATH "Software\Microsoft\Windows\CurrentVersion\Uninstall\DeskPins"
!define RUN_PATH    "Software\Microsoft\Windows\CurrentVersion\Run"
!define CM_CLOSE    11844   ;; from resource.h

Name "DeskPins ${VERSION}"
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES64\DeskPins"
InstallDirRegKey HKLM "${APP_KEY}" "Install_Dir"
XPStyle on

VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=0 ProductName     "DeskPins"
VIAddVersionKey /LANG=0 CompanyName     "Elias Fotinis"
VIAddVersionKey /LANG=0 LegalCopyright  "Copyright (C) 2002-2015 Elias Fotinis"
VIAddVersionKey /LANG=0 FileDescription "DeskPins installer"
VIAddVersionKey /LANG=0 FileVersion     "${VERSION}"
VIAddVersionKey /LANG=0 ProductVersion  "${VERSION}"

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles


;; ----------------
;; Languages (the one matching the Windows display language is picked automatically)
;; ----------------

LoadLanguageFile "${NSISDIR}\Contrib\Language files\English.nlf"
LoadLanguageFile "${NSISDIR}\Contrib\Language files\SimpChinese.nlf"

LangString SEC_CORE    ${LANG_ENGLISH}     "DeskPins (required)"
LangString SEC_CORE    ${LANG_SIMPCHINESE} "DeskPins（必需）"
LangString SEC_STARTMENU ${LANG_ENGLISH}     "Start menu shortcuts"
LangString SEC_STARTMENU ${LANG_SIMPCHINESE} "开始菜单快捷方式"
LangString SEC_DESKTOP ${LANG_ENGLISH}     "Desktop shortcut"
LangString SEC_DESKTOP ${LANG_SIMPCHINESE} "添加快捷方式到桌面"
LangString SEC_STARTUP ${LANG_ENGLISH}     "Start DeskPins with Windows"
LangString SEC_STARTUP ${LANG_SIMPCHINESE} "开机时自动运行 DeskPins"
LangString LNK_HELP    ${LANG_ENGLISH}     "Help"
LangString LNK_HELP    ${LANG_SIMPCHINESE} "帮助"
LangString LNK_UNINST  ${LANG_ENGLISH}     "Uninstall"
LangString LNK_UNINST  ${LANG_SIMPCHINESE} "卸载"
LangString MSG_NEEDX64 ${LANG_ENGLISH}     "This version of DeskPins requires 64-bit Windows."
LangString MSG_NEEDX64 ${LANG_SIMPCHINESE} "此版本的 DeskPins 需要 64 位 Windows。"


;; Ask a running DeskPins to close (it saves its options on exit).
!macro CloseRunningApp
    FindWindow $0 "EFDeskPins"
    ${If} $0 <> 0
        SendMessage $0 ${WM_COMMAND} ${CM_CLOSE} 0 /TIMEOUT=5000
        Sleep 500
    ${EndIf}
!macroend


Function .onInit
    ${IfNot} ${RunningX64}
        MessageBox MB_ICONSTOP "$(MSG_NEEDX64)"
        Abort
    ${EndIf}
    SetRegView 64
    ;; per-machine install: shortcuts go to the all-users Start menu / desktop
    SetShellVarContext all
FunctionEnd


Function un.onInit
    SetRegView 64
    SetShellVarContext all
FunctionEnd


Section "$(SEC_CORE)"
    SectionIn RO

    !insertmacro CloseRunningApp

    SetOutPath "$INSTDIR"
    File "${BINDIR}\DeskPins.exe"
    File "..\help\DeskPins.chm"
    File "..\license.txt"

    WriteRegStr HKLM "${APP_KEY}" "Install_Dir" "$INSTDIR"

    WriteRegStr   HKLM "${UNINST_PATH}" "DisplayName"     "DeskPins"
    WriteRegStr   HKLM "${UNINST_PATH}" "DisplayVersion"  "${VERSION}"
    WriteRegStr   HKLM "${UNINST_PATH}" "Publisher"       "Elias Fotinis"
    WriteRegStr   HKLM "${UNINST_PATH}" "DisplayIcon"     '"$INSTDIR\DeskPins.exe"'
    WriteRegStr   HKLM "${UNINST_PATH}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKLM "${UNINST_PATH}" "UninstallString" '"$INSTDIR\uninst.exe"'
    WriteRegDWORD HKLM "${UNINST_PATH}" "NoModify" 1
    WriteRegDWORD HKLM "${UNINST_PATH}" "NoRepair" 1

    WriteUninstaller "$INSTDIR\uninst.exe"
SectionEnd


Section "$(SEC_STARTMENU)"
    CreateDirectory "$SMPROGRAMS\DeskPins"
    CreateShortCut "$SMPROGRAMS\DeskPins\DeskPins.lnk"        "$INSTDIR\DeskPins.exe"
    CreateShortCut "$SMPROGRAMS\DeskPins\$(LNK_HELP).lnk"     "$INSTDIR\DeskPins.chm"
    CreateShortCut "$SMPROGRAMS\DeskPins\$(LNK_UNINST).lnk"   "$INSTDIR\uninst.exe"
SectionEnd


Section "$(SEC_DESKTOP)"
    CreateShortCut "$DESKTOP\DeskPins.lnk" "$INSTDIR\DeskPins.exe"
SectionEnd


Section "$(SEC_STARTUP)"
    WriteRegStr HKCU "${RUN_PATH}" "DeskPins" '"$INSTDIR\DeskPins.exe"'
SectionEnd


Section "Uninstall"
    !insertmacro CloseRunningApp

    DeleteRegKey HKLM "${UNINST_PATH}"
    DeleteRegValue HKCU "${RUN_PATH}" "DeskPins"

    DeleteRegKey HKLM "${APP_KEY}"
    DeleteRegKey HKCU "${APP_KEY}"
    DeleteRegKey /ifempty HKLM "Software\Elias Fotinis"
    DeleteRegKey /ifempty HKCU "Software\Elias Fotinis"

    Delete "$INSTDIR\DeskPins.exe"
    Delete "$INSTDIR\DeskPins*.chm"
    Delete "$INSTDIR\license.txt"
    Delete "$INSTDIR\uninst.exe"
    ;; legacy files from older versions
    Delete "$INSTDIR\dphook.dll"
    Delete "$INSTDIR\lang*.dll"
    Delete "$INSTDIR\readme*.txt"

    Delete "$DESKTOP\DeskPins.lnk"
    Delete "$SMPROGRAMS\DeskPins\*.lnk"
    RMDir "$SMPROGRAMS\DeskPins"
    RMDir "$INSTDIR"
SectionEnd
