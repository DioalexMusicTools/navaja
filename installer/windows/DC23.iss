#define AppTitle "DC-23"
#define AppVersion "1.0.0"

[Setup]
AppId=DC23_VST3_DioalexMusicTools
AppName={#AppTitle}
AppVersion={#AppVersion}
AppPublisher=Dioalex Music Tools
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
UninstallFilesDir={autopf}\Dioalex Music Tools\DC-23
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\..\dist
OutputBaseFilename=DC-23-v1.0-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes

[Files]
Source: "payload\DC-23.vst3\*"; DestDir: "{app}\DC-23.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

Source: "Uninstall DC-23.cmd"; DestDir: "{autopf}\Dioalex Music Tools\DC-23"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\Dioalex Music Tools\Uninstall DC-23"; Filename: "{uninstallexe}"

[Run]
; FL Studio will rescan its plugin list the next time it opens.

[UninstallDelete]
Type: dirifempty; Name: "{app}\DC-23.vst3"
