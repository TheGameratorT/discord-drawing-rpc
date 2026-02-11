# Flatpak Build

This directory contains the Flatpak manifest and metadata for Discord Drawing RPC.

## Files

- `com.TheGameratorT.DiscordDrawingRPC.yml.in` - Manifest template
- `com.TheGameratorT.DiscordDrawingRPC.metainfo.xml.in` - AppStream metadata template
- `generate_manifest.cmake` - CMake script to generate manifests

## Generating Manifests

The project uses a template-based system to generate different Flatpak manifests.

### CI Manifest (for local development, CI/CD, and releases)

Generate a manifest that uses the local source directory:

```bash
cmake -DMANIFEST_TYPE=CI \
      -DAPP_VERSION_STRING=$(git rev-parse --short HEAD) \
      -DSOURCE_DIR=$PWD \
      -DOUTPUT_DIR=$PWD/build/flatpak \
      -P flatpak/generate_manifest.cmake
```

This creates `build/flatpak/com.TheGameratorT.DiscordDrawingRPC.yml` that:
- Uses the specified version (git hash or release tag)
- Sources from the local directory (no git clone)
- Used for CI pipelines, local testing, and GitHub releases

### Publishing Manifest (for Flathub submission)

Generate a manifest for Flathub that references a git tag:

```bash
cmake -DMANIFEST_TYPE=PUBLISHING \
      -DAPP_VERSION_STRING=1.1.1 \
      -DSOURCE_DIR=$PWD \
      -DOUTPUT_DIR=$PWD/build/flatpak \
      -P flatpak/generate_manifest.cmake
```

This creates `build/flatpak/com.TheGameratorT.DiscordDrawingRPC.yml` that:
- Uses the specified version number
- Sources from the git repository at tag `v{VERSION}`
- Required for Flathub submissions

## Building Locally

### Prerequisites

Install flatpak-builder and the KDE runtime:
```bash
# Install flatpak-builder
sudo apt install flatpak-builder  # Ubuntu/Debian
sudo dnf install flatpak-builder  # Fedora

# Add Flathub repository
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

# Install KDE Platform
flatpak install flathub org.kde.Platform//6.10 org.kde.Sdk//6.10
```

### Build from CI Manifest

First generate the CI manifest, then build:

```bash
# Generate manifest
cmake -DMANIFEST_TYPE=CI \
      -DAPP_VERSION_STRING=$(git rev-parse --short HEAD) \
      -DSOURCE_DIR=$PWD \
      -DOUTPUT_DIR=$PWD/build/flatpak \
      -P flatpak/generate_manifest.cmake

# Build
flatpak-builder --force-clean --install-deps-from=flathub \
  flatpak-build-dir build/flatpak/com.TheGameratorT.DiscordDrawingRPC.yml
```

Run the application without installing:
```bash
flatpak-builder --run flatpak-build-dir \
  build/flatpak/com.TheGameratorT.DiscordDrawingRPC.yml discord-drawing-rpc
```

Install locally for testing:
```bash
flatpak-builder --user --install --force-clean flatpak-build-dir \
  build/flatpak/com.TheGameratorT.DiscordDrawingRPC.yml
flatpak run com.TheGameratorT.DiscordDrawingRPC
```

## Creating a Flatpak Bundle (for distribution)

To create a `.flatpak` bundle file for direct distribution:

```bash
# Generate CI manifest with release version
cmake -DMANIFEST_TYPE=CI \
      -DAPP_VERSION_STRING=1.1.1 \
      -DSOURCE_DIR=$PWD \
      -DOUTPUT_DIR=$PWD/build/flatpak \
      -P flatpak/generate_manifest.cmake

# Build and create bundle
flatpak-builder --repo=repo --force-clean flatpak-build-dir \
  build/flatpak/com.TheGameratorT.DiscordDrawingRPC.yml
flatpak build-bundle repo discord-drawing-rpc.flatpak com.TheGameratorT.DiscordDrawingRPC

# Users can install with:
flatpak install discord-drawing-rpc.flatpak
```

## CI/CD Integration

For CI pipelines, generate the CI manifest which uses local sources:

```yaml
# Example GitHub Actions workflow snippet
- name: Generate Flatpak manifest
  run: |
    cmake -DMANIFEST_TYPE=CI \
          -DAPP_VERSION_STRING=$(git rev-parse --short HEAD) \
          -DSOURCE_DIR=$PWD \
          -DOUTPUT_DIR=$PWD/build/flatpak \
          -P flatpak/generate_manifest.cmake
    
- name: Build Flatpak
  run: |
    flatpak-builder --force-clean --install-deps-from=flathub \
      flatpak-build-dir build/flatpak/com.TheGameratorT.DiscordDrawingRPC.yml
```

## Publishing to Flathub

**For Flathub submission, you submit the manifest file, NOT a .flatpak bundle.**

1. **Generate the publishing manifest:**
   ```bash
   cmake -DMANIFEST_TYPE=PUBLISHING \
         -DAPP_VERSION_STRING=1.1.1 \
         -DSOURCE_DIR=$PWD \
         -DOUTPUT_DIR=$PWD/build/flatpak \
         -P flatpak/generate_manifest.cmake
   ```

   This creates a manifest with a git source pointing to tag `v1.1.1`.

2. **Submit to Flathub:**
   - Create a new repository on GitHub under the Flathub organization: `https://github.com/flathub/com.TheGameratorT.DiscordDrawingRPC`
   - Copy the generated manifest (`build/flatpak/com.TheGameratorT.DiscordDrawingRPC.yml`) to this repository
   - Copy the metainfo file if needed
   - Submit a pull request to `flathub/flathub` to request app inclusion
   - Follow the [Flathub documentation](https://docs.flathub.org/)

3. **Updating on Flathub:**
   - Tag a new release in your git repository
   - Generate a new publishing manifest with the updated version
   - Update the manifest in the Flathub repository
   - Flathub's CI will automatically build and publish the update

## Configuration

The application uses XDG directories for configuration:
- Config: `~/.var/app/com.discorddrawingrpc.DiscordDrawingRPC/config/discord-drawing-rpc/config.json`
- Data: `~/.var/app/com.discorddrawingrpc.DiscordDrawingRPC/data/discord-drawing-rpc/`

These are automatically handled by the Flatpak sandbox and the application's existing XDG-compliant code.
