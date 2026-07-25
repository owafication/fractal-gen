#ifndef SourceExe
  #define SourceExe "..\build\Release\MandelbrotWallpaper.exe"
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif

#define AppName "Mandelbrot Live Wallpaper"
#define AppVersion "1.11.7"
#define AppPublisher "Mandelbrot Live Wallpaper contributors"
#define AppExeName "MandelbrotWallpaper.exe"

[Setup]
AppId={{20F1F29B-52A3-4BEE-AB7E-BCE7F6100C39}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={localappdata}\Programs\MandelbrotLiveWallpaper
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename=Mandelbrot-Live-Wallpaper-{#AppVersion}-Setup-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#AppExeName}
SetupIconFile=..\assets\icons\mandelbrot.ico
LicenseFile=..\LICENSE
CloseApplications=yes
RestartApplications=no
VersionInfoVersion={#AppVersion}
VersionInfoDescription={#AppName} Setup

[Files]
Source: "{#SourceExe}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\*"; DestDir: "{app}\docs"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\assets\presets\*"; DestDir: "{app}\assets\presets"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{userdesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{cmd}"; Parameters: "/C taskkill /IM {#AppExeName} /T /F >NUL 2>&1"; Flags: runhidden; RunOnceId: "StopWallpaperProcess"

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
