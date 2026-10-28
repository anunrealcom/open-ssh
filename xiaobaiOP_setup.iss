; xiaobaiOP 安装包脚本（Inno Setup 6）
; 用法：安装 Inno Setup 后，右键本文件 → Compile，
;        或命令行：ISCC.exe xiaobaiOP_setup.iss
; 输出：installer/xiaobaiOP_setup_vX.X.X.exe

#define MyAppName "小白运维 xiaobaiOP"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "xiaobaiOP"
#define MyAppURL "https://github.com/yourname/xiaobaiOP"
#define MyAppExeName "xiaobaiOP.exe"

[Setup]
AppId={{8F3C2A1B-5D6E-4C7A-9B0F-1E2D3C4B5A60}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=installer
OutputBaseFilename=xiaobaiOP_setup_v{#MyAppVersion}
SetupIconFile=
Compression=lzma2
SolidCompression=yes
; 不要加壳、不要 UPX，避免触发杀软启发式
ChangesAssociations=no
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64
WizardStyle=modern
; 卸载时清理
UninstallDisplayName={#MyAppName}

[Languages]
Name: "chinese"; MessagesFile: "compiler:Default.isl"
; 如需中文界面，Inno Setup 自带中文消息可改为 compiler:Languages\ChineseSimplified.isl

[Files]
; dist 目录由 deploy_release.bat 生成（windeployqt 收集的依赖）
Source: "dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加任务:"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "安装完成后启动 {#MyAppName}"; Flags: nowait postinstall skipifsilent
