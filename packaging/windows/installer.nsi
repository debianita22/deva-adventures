; Deva's Awesome Adventures - the Windows installer (1.2), built by tools/release/mkdist.py:
;   makensis -DVERSION=1.2.0 -DSRC=<the unpacked Windows package> -DOUT=<setup.exe> installer.nsi
;
; For this user only, no administrator: the game goes to %LOCALAPPDATA%\Programs\Deva's Awesome
; Adventures, with a shortcut in the Start menu and one on the desktop, and an entry in "App
; installate" to remove it. The saves stay where the game keeps them (%APPDATA%\deva-adventures),
; also when the game is removed. A deva_adventures.cfg already there is the grown-ups': it stays,
; and the new defaults go beside it as deva_adventures.cfg.default.
; Silent: setup.exe /S; uninstall.exe /S.

Target amd64-unicode ; (a 64-bit installer for a 64-bit game)
SetCompressor /SOLID lzma
SetDateSave off ; (no file times inside: the same package gives the same installer)
RequestExecutionLevel user
ManifestDPIAware true

!ifndef VERSION
!error "VERSION is needed: see the top of this file"
!endif
!ifndef SRC
!error "SRC is needed: see the top of this file"
!endif
!ifndef OUT
!error "OUT is needed: see the top of this file"
!endif

!define APP "Deva's Awesome Adventures"
!define EXE "deva-adventures.exe"
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\DevaAdventures"

Name "${APP}"
OutFile "${OUT}"
InstallDir "$LOCALAPPDATA\Programs\${APP}"
InstallDirRegKey HKCU "${UNINST_KEY}" "InstallLocation"
BrandingText "${APP} ${VERSION}"
ShowInstDetails nevershow
ShowUninstDetails nevershow

!include "MUI2.nsh"
!include "FileFunc.nsh"

!define MUI_ICON "${SRC}\deva-adventures.ico"
!define MUI_UNICON "${SRC}\deva-adventures.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "installer.bmp" ; (tools/art/make_icon.py: the menu's card and Deva)
!define MUI_WELCOMEPAGE_TITLE "${APP} ${VERSION}"
!define MUI_WELCOMEPAGE_TEXT "Diciotto giochi e quattro storie con la voce in italiano, per bambini di 5 anni.$\r$\n$\r$\nIl gioco si installa per questo utente (non servono permessi da amministratore). I salvataggi restano anche aggiornando o togliendo il gioco."
!define MUI_FINISHPAGE_RUN "$INSTDIR\${EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Gioca adesso"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\docs\manuale.pdf"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Apri il manuale per i genitori"
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Italian"

VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=${LANG_ITALIAN} "ProductName" "${APP}"
VIAddVersionKey /LANG=${LANG_ITALIAN} "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=${LANG_ITALIAN} "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=${LANG_ITALIAN} "FileDescription" "Installazione di ${APP}"
VIAddVersionKey /LANG=${LANG_ITALIAN} "LegalCopyright" "Licenza MIT"

Section "Gioco"
    SetOutPath "$INSTDIR"
    ; the data of the version before go first (a file dropped from the game would stay), the
    ; grown-ups' settings aside
    IfFileExists "$INSTDIR\deva_adventures\deva_adventures.cfg" 0 +2
        Rename "$INSTDIR\deva_adventures\deva_adventures.cfg" "$INSTDIR\deva_adventures.cfg.keep"
    RMDir /r "$INSTDIR\deva_adventures"
    File "${SRC}\${EXE}"
    File "${SRC}\SDL2.dll"
    File "${SRC}\deva-adventures.png"
    File "${SRC}\deva-adventures.ico"
    File "${SRC}\LEGGIMI.txt"
    File "${SRC}\LICENSE"
    File "${SRC}\LICENSE-SDL2.txt"
    File "${SRC}\LICENSE-mingw-w64.txt"
    File "${SRC}\THIRD_PARTY.md"
    File "${SRC}\CHANGELOG.md"
    File "${SRC}\SHA256SUMS"
    File /r "${SRC}\docs"
    File /r "${SRC}\tools"
    File /r "${SRC}\deva_adventures"
    IfFileExists "$INSTDIR\deva_adventures.cfg.keep" 0 settings_done
        Rename "$INSTDIR\deva_adventures\deva_adventures.cfg" "$INSTDIR\deva_adventures\deva_adventures.cfg.default"
        Rename "$INSTDIR\deva_adventures.cfg.keep" "$INSTDIR\deva_adventures\deva_adventures.cfg"
    settings_done:

    WriteUninstaller "$INSTDIR\uninstall.exe"
    CreateShortcut "$SMPROGRAMS\${APP}.lnk" "$INSTDIR\${EXE}" "--fullscreen" "$INSTDIR\deva-adventures.ico" 0
    CreateShortcut "$DESKTOP\${APP}.lnk" "$INSTDIR\${EXE}" "--fullscreen" "$INSTDIR\deva-adventures.ico" 0

    WriteRegStr HKCU "${UNINST_KEY}" "DisplayName" "${APP}"
    WriteRegStr HKCU "${UNINST_KEY}" "DisplayVersion" "${VERSION}"
    WriteRegStr HKCU "${UNINST_KEY}" "Publisher" "Deva's Awesome Adventures"
    WriteRegStr HKCU "${UNINST_KEY}" "DisplayIcon" "$INSTDIR\deva-adventures.ico"
    WriteRegStr HKCU "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "${UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKCU "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
    WriteRegDWORD HKCU "${UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKCU "${UNINST_KEY}" "NoRepair" 1
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKCU "${UNINST_KEY}" "EstimatedSize" "$0"
SectionEnd

Section "Uninstall"
    Delete "$SMPROGRAMS\${APP}.lnk"
    Delete "$DESKTOP\${APP}.lnk"
    RMDir /r "$INSTDIR\deva_adventures"
    RMDir /r "$INSTDIR\docs"
    RMDir /r "$INSTDIR\tools"
    Delete "$INSTDIR\${EXE}"
    Delete "$INSTDIR\SDL2.dll"
    Delete "$INSTDIR\deva-adventures.png"
    Delete "$INSTDIR\deva-adventures.ico"
    Delete "$INSTDIR\LEGGIMI.txt"
    Delete "$INSTDIR\LICENSE"
    Delete "$INSTDIR\LICENSE-SDL2.txt"
    Delete "$INSTDIR\LICENSE-mingw-w64.txt"
    Delete "$INSTDIR\THIRD_PARTY.md"
    Delete "$INSTDIR\CHANGELOG.md"
    Delete "$INSTDIR\SHA256SUMS"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"
    DeleteRegKey HKCU "${UNINST_KEY}"
SectionEnd
