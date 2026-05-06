#define MyAppName "BlinkText"
#define MyAppVersion "1.1.0"
#define MyAppPublisher "BlinkText"
#define MyAppExeName "BlinkText.exe"

[Setup]
AppId={{B7D0E6B9-7B7D-4E7A-8A2D-BLINKTEXT001}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=installer_output
OutputBaseFilename=BlinkText_Setup_v{#MyAppVersion}
SetupIconFile=src\assets\app.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
UninstallDisplayIcon={app}\{#MyAppExeName}
DisableProgramGroupPage=yes
DisableReadyPage=no
DisableFinishedPage=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
Source: "build_release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\BlinkText"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\BlinkText"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch BlinkText"; Flags: nowait postinstall skipifsilent
