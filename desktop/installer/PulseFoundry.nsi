Unicode True
!include "MUI2.nsh"
!include "x64.nsh"
!ifndef STAGE
  !error "Define STAGE with the prepared release directory"
!endif
!ifndef OUTPUT
  !define OUTPUT "PulseFoundry-0.1.0-Windows-x64-Setup.exe"
!endif
Name "Pulse Foundry 0.1.0"
OutFile "${OUTPUT}"
InstallDir "$PROGRAMFILES64\Pulse Foundry"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${STAGE}/LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "Pulse Foundry requires 64-bit Windows."
    Abort
  ${EndIf}
  SetRegView 64
FunctionEnd
Section "Pulse Foundry" SEC_MAIN
  SetOutPath "$INSTDIR"
  File "${STAGE}/Pulse Foundry.exe"
  File "${STAGE}/LICENSE.txt"
  File "${STAGE}/NOTICE.md"
  File "${STAGE}/README.txt"
  File "${STAGE}/THIRD_PARTY.md"
  SetOutPath "$INSTDIR\Source"
  File "${STAGE}/PulseFoundry-source.zip"
  SetOutPath "$COMMONFILES64\VST3\Pulse Foundry.vst3"
  File /r "${STAGE}/Pulse Foundry.vst3/*"
  SetOutPath "$INSTDIR"
  CreateDirectory "$SMPROGRAMS\Pulse Foundry"
  CreateShortcut "$SMPROGRAMS\Pulse Foundry\Pulse Foundry.lnk" "$INSTDIR\Pulse Foundry.exe"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateShortcut "$SMPROGRAMS\Pulse Foundry\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PulseFoundry" "DisplayName" "Pulse Foundry"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PulseFoundry" "DisplayVersion" "0.1.0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PulseFoundry" "UninstallString" '"$INSTDIR\Uninstall.exe"'
SectionEnd
Section "Uninstall"
  SetRegView 64
  Delete "$INSTDIR\Pulse Foundry.exe"
  Delete "$INSTDIR\LICENSE.txt"
  Delete "$INSTDIR\NOTICE.md"
  Delete "$INSTDIR\README.txt"
  Delete "$INSTDIR\THIRD_PARTY.md"
  Delete "$INSTDIR\Source\PulseFoundry-source.zip"
  RMDir "$INSTDIR\Source"
  RMDir /r "$COMMONFILES64\VST3\Pulse Foundry.vst3"
  Delete "$SMPROGRAMS\Pulse Foundry\Pulse Foundry.lnk"
  Delete "$SMPROGRAMS\Pulse Foundry\Uninstall.lnk"
  RMDir "$SMPROGRAMS\Pulse Foundry"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PulseFoundry"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
SectionEnd
