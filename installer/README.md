# Discord Draw RPC - Installer Guide

## Prerequisites

To build the Windows installer, you need:

1. **Qt Installer Framework (QtIFW)**: Download from https://download.qt.io/official_releases/qt-installer-framework/
   - Recommended version: 4.5 or later
   - Install to default location (C:/Qt/QtIFW-x.x.x) or set `QTIFWDIR` environment variable

## Building the Installer

### Step 1: Build the Application
```bash
cmake -B build -G Ninja
cmake --build build
```

This will:
- Compile all executables (DiscordDrawingRPC.exe, DiscordDrawingRPCDaemon.exe, DiscordDrawingRPCTray.exe)
- Copy them to `build/dist/`
- Deploy Qt DLLs and MinGW dependencies to `build/dist/`

### Step 2: Create the Installer
```bash
cmake --build build --target installer
```

This will:
- Package all contents from `build/dist/` into the installer
- Create `build/DiscordDrawingRPC-Setup.exe`

## Installer Features

### What gets installed:
- **DiscordDrawingRPC.exe** - Main GUI application (with Start Menu & Desktop shortcuts)
- **DiscordDrawingRPCDaemon.exe** - Background daemon (internal, no shortcut)
- **DiscordDrawingRPCTray.exe** - System tray application (internal, no shortcut)
- All required Qt DLLs and dependencies

### Installation locations:
- Default: `C:/Program Files/DiscordDrawingRPC/`
- Start Menu: `Discord Draw RPC` folder with shortcut to main app
- Desktop: `Discord Draw RPC.lnk` shortcut (optional)

### Uninstall:
- Via Windows "Add or Remove Programs"
- Via `MaintenanceTool.exe` in installation directory

## Customization

### Modify installer appearance:
Edit `installer/config/config.xml`:
- Change application name, version, publisher
- Adjust wizard size and style

### Modify package content:
Edit `installer/packages/com.DiscordDrawingRPC.app/meta/package.xml`:
- Update version, release date, description
- Change license

### Modify shortcuts and operations:
Edit `installer/packages/com.DiscordDrawingRPC.app/meta/installscript.qs`:
- Add/remove shortcuts
- Add registry entries
- Customize installation operations

## Troubleshooting

### "binarycreator not found"
- Install Qt Installer Framework from https://download.qt.io/official_releases/qt-installer-framework/
- Add QtIFW bin directory to PATH, or set QTIFWDIR environment variable

### Installer includes wrong files
- Check `build/dist/` folder contents before building installer
- Clean and rebuild if necessary: `cmake --build build --target clean`

### Missing DLLs in installer
- Ensure `windeployqt` ran successfully during the build
- Check build output for MinGW DLL copy commands
- Manually copy missing DLLs to `build/dist/` and rebuild installer

## Manual Build (Alternative)

If you prefer to build manually:

```bash
# From project root
cd installer
binarycreator --offline-only -c config/config.xml -p packages ../build/DiscordDrawingRPC-Setup.exe
```

Make sure `build/dist/` contents are copied to `packages/com.DiscordDrawingRPC.app/data/` first.
