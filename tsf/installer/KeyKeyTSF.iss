[Setup]
AppId={{205BC078-BF4D-45D3-837E-CDA0395C066C}
AppName=Yahoo! KeyKey TSF
AppVersion=1.0.4
AppVerName=Yahoo! KeyKey TSF 1.0.4
AppPublisher=KeyKey TSF Bridge
AppPublisherURL=https://github.com/able-bodied-man/yahoo_key_key
AppSupportURL=https://github.com/able-bodied-man/yahoo_key_key/issues
AppUpdatesURL=https://github.com/able-bodied-man/yahoo_key_key/releases
DefaultDirName={autopf}\Yahoo! KeyKey TSF
DefaultGroupName=Yahoo! KeyKey TSF
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE.txt
OutputDir=..\release
OutputBaseFilename=KeyKeyTSF-1.0.4-x64-Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
SetupIconFile=..\KeyKeyTSF\Yahoo.ico
UninstallDisplayIcon={app}\KeyKeyTSF.dll
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\KeyKeyTSF\build\KeyKeyTSF.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\install.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\uninstall.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\INSTALL.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "keykey64.msi"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
Name: "{group}\Yahoo! KeyKey TSF Readme"; Filename: "{app}\INSTALL.txt"
Name: "{group}\Uninstall Yahoo! KeyKey TSF"; Filename: "{uninstallexe}"

[UninstallRun]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\uninstall.ps1"" -InstallDir ""{app}"""; Flags: waituntilterminated runhidden; RunOnceId: "RemoveKeyKeyTSF"

[Code]
function KeyKeyInstalled(): Boolean;
var
  Location: String;
begin
  Result :=
    (RegQueryStringValue(HKLM64, 'SOFTWARE\Yahoo\KeyKey', 'ServerLocation', Location) and
     FileExists(AddBackslash(Location) + 'KeyKeyServer.exe')) or
    (RegQueryStringValue(HKLM32, 'SOFTWARE\Yahoo\KeyKey', 'ServerLocation', Location) and
     FileExists(AddBackslash(Location) + 'KeyKeyServer.exe'));
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
  Params: String;
begin
  if CurStep = ssPostInstall then
  begin
    Params := '-NoProfile -ExecutionPolicy Bypass -File "' + ExpandConstant('{app}\install.ps1') +
              '" -InstallDir "' + ExpandConstant('{app}') + '"';

    if not KeyKeyInstalled() then
    begin
      Params := Params + ' -KeyKeyMsi "' + ExpandConstant('{tmp}\keykey64.msi') + '"';
    end;

    if not Exec(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'), Params, '',
                SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    begin
      MsgBox('Could not start the KeyKey TSF installer.', mbError, MB_OK);
      Abort;
    end;

    if ResultCode <> 0 then
    begin
      MsgBox('KeyKey TSF installation failed with exit code ' + IntToStr(ResultCode) +
             '.' + #13#10#13#10 +
             'See %TEMP%\keykeytsf-install.log and %TEMP%\keykeytsf-install-error.txt for details.',
             mbError, MB_OK);
      Abort;
    end;
  end;
end;

