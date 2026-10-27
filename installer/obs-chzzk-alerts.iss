#ifndef MyAppVersion
  #define MyAppVersion "0.3.0"
#endif
#ifndef BuildRoot
  #define BuildRoot "..\release\Release\obs-chzzk-alerts"
#endif
#ifndef OutputRoot
  #define OutputRoot "..\release"
#endif

#define MyAppName "OBS Chzzk Alerts"
#define MyAppPublisher "OBS Chzzk Alerts contributors"
#define MyAppExeName "obs64.exe"

[Setup]
AppId={{7D5887BA-B71E-4899-BC97-5A8EAE6B33F5}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\OBS Chzzk Alerts
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir={#OutputRoot}
OutputBaseFilename=obs-chzzk-alerts-{#MyAppVersion}-windows-x64-setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=force
RestartApplications=no
UninstallDisplayName={#MyAppName}
VersionInfoVersion={#MyAppVersion}
VersionInfoProductName={#MyAppName}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription=CHZZK follow alert plugin for OBS Studio

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#BuildRoot}\bin\64bit\obs-chzzk-alerts.dll"; DestDir: "{code:GetObsPath}\obs-plugins\64bit"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "{#BuildRoot}\data\*"; DestDir: "{code:GetObsPath}\data\obs-plugins\obs-chzzk-alerts"; Flags: ignoreversion recursesubdirs createallsubdirs

[InstallDelete]
Type: filesandordirs; Name: "{code:GetObsPath}\data\obs-plugins\obs-chzzk-alerts\browser-source"

[Run]
Filename: "{code:GetObsPath}\bin\64bit\{#MyAppExeName}"; Description: "{cm:LaunchProgram,OBS Studio}"; WorkingDir: "{code:GetObsPath}\bin\64bit"; Flags: nowait postinstall skipifsilent runasoriginaluser

[Code]
var
  CachedObsPath: string;

function DetectObsPath(): string;
var
  UninstallCommand: string;
  Candidate: string;
begin
  Result := '';
  if RegQueryStringValue(HKLM32,
      'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OBS Studio',
      'UninstallString', UninstallCommand) then
  begin
    Candidate := ExtractFileDir(RemoveQuotes(UninstallCommand));
    if FileExists(AddBackslash(Candidate) + 'bin\64bit\{#MyAppExeName}') then
      Result := Candidate;
  end;

  if Result = '' then
  begin
    Candidate := ExpandConstant('{autopf}\obs-studio');
    if FileExists(AddBackslash(Candidate) + 'bin\64bit\{#MyAppExeName}') then
      Result := Candidate;
  end;
end;

function GetObsPath(Param: string): string;
begin
  if CachedObsPath = '' then
    CachedObsPath := DetectObsPath();
  Result := CachedObsPath;
end;

function InitializeSetup(): Boolean;
var
  OldPluginPath: string;
begin
  CachedObsPath := DetectObsPath();
  Result := CachedObsPath <> '';
  if not Result then
  begin
    MsgBox('64비트 OBS Studio 설치를 찾지 못했습니다. OBS를 먼저 설치해 주세요.', mbError, MB_OK);
    exit;
  end;

  OldPluginPath := AddBackslash(CachedObsPath) + 'obs-plugins\64bit\obs-live-editor.dll';
  if FileExists(OldPluginPath) then
    MsgBox('OBS Live Editor도 설치되어 있습니다. 포트 충돌을 방지하려면 Live Editor 0.3.2 이상을 사용해 주세요.', mbInformation, MB_OK);
end;
