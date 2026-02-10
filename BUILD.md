# Build Instructions

## Prerequisites

- **CMake** (version 3.16 or higher)
- **Qt6** with the following modules:
  - Qt6::Core
  - Qt6::Widgets
  - Qt6::Network
- **C++17** compatible compiler
- **Ninja** (recommended) or another CMake-supported build system

## Building on Windows

**Note:** Building on Windows requires using the MINGW console with qt6-base and qt6-tools installed.

1. Install dependencies via MINGW:
   ```bash
   # Install required packages in MINGW console
   pacman -S mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-tools mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
   ```

2. Clone the repository:
   ```bash
   git clone <repository-url>
   cd discord-draw-rpc
   ```

3. Build the project:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

4. Build the installer:
   ```bash
   cmake --build . --target installer
   ```

5. The executables will be generated in the build directory:
   - `DiscordDrawingRPC.exe` – Main GUI application
   - `DiscordDrawingRPCDaemon.exe` – Background daemon
   - `DiscordDrawingRPCTray.exe` – System tray application

## Building on Linux/macOS

1. Install dependencies:
   ```bash
   # Ubuntu/Debian
   sudo apt install cmake qt6-base-dev qt6-tools-dev ninja-build

   # macOS (using Homebrew)
   brew install cmake qt@6 ninja
   ```

2. Clone and build:
   ```bash
   git clone <repository-url>
   cd discord-draw-rpc
   mkdir build && cd build
   cmake -G "Ninja" ..
   cmake --build .
   ```

## CMake Options

You can customize the build with CMake options:

```bash
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
```

## Running

After building, you can run the applications from the build directory:

**Linux/macOS:**
- Start the daemon first: `./discord-drawing-rpc-daemon`
- Run the GUI: `./discord-drawing-rpc`
- Or use the tray application: `./discord-drawing-rpc-tray`

**Windows:**
- Start the daemon first: `./DiscordDrawingRPCDaemon.exe`
- Run the GUI: `./DiscordDrawingRPC.exe`
- Or use the tray application: `./DiscordDrawingRPCTray.exe`

## Installer

An installer can be built using the scripts in the `installer/` directory. See [installer/README.md](installer/README.md) for details.

## Troubleshooting

### Qt6 not found

Make sure Qt6 is installed and the `CMAKE_PREFIX_PATH` is set:

```bash
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt6 ..
```

### Build errors

- Ensure you have a C++17 compatible compiler
- Check that all Qt6 modules are installed
- Try cleaning the build directory and reconfiguring
