# Nex CLI Usage Guide

## Installation

### Download

Download the latest release for your platform from the [Releases page](https://github.com/devkiraa/nex/releases):

- **Windows:** `nex-windows-x64.exe`
- **Linux:** `nex-linux-x64`
- **macOS:** `nex-macos-x64`

### Add to PATH

#### Windows

1. Create a folder (e.g., `C:\nex`)
2. Move `nex.exe` to that folder
3. Add the folder to your PATH:
   - Open System Properties → Advanced → Environment Variables
   - Under "User variables" or "System variables", find "Path"
   - Click Edit → New → Add `C:\nex`
   - Click OK to save

#### Linux / macOS

```bash
# Move to a directory in PATH
sudo mv nex /usr/local/bin/
sudo chmod +x /usr/local/bin/nex
```

### Verify Installation

```bash
nex --version
# Output: nex 1.0.0
```

## Commands

### Installing Packages

```bash
nex install <package-id>
```

Example:
```bash
nex install example.hello-world
```

This will:
1. Fetch the package manifest from the registry
2. Clone the package repository to `~/.nex/packages/<package-id>/`
3. Run any install scripts defined in the manifest

### Running Packages

```bash
# Run default command
nex run <package-id>

# Run specific command
nex run <package-id> <command-name>

# Pass arguments
nex run <package-id> <command-name> [args...]
```

Examples:
```bash
# Run default command
nex run example.hello-world

# Run specific command
nex run john.image-converter convert --input file.png --output file.jpg

# Run with arguments
nex run data.processor analyze file.csv --format json
```

### Searching Packages

```bash
nex search <query>
```

Examples:
```bash
nex search python
nex search "image converter"
nex search automation
```

### Listing Installed Packages

```bash
nex list
```

Output:
```
Installed packages:

PACKAGE                                  VERSION        
-------                                  -------        
example.hello-world                      1.0.0          
john.image-converter                     2.1.0          

Total: 2 package(s)
```

### Package Information

```bash
nex info <package-id>
```

Example:
```bash
nex info example.hello-world
```

Output:
```
Package: Hello World
ID:      example.hello-world
Version: 1.0.0
Author:  Nex Team

Description:
  A simple example package demonstrating nex structure.

Runtime: Python >=3.6
Repository: https://github.com/devkiraa/example-hello-world

Commands:
  default: python main.py
  greet: python main.py greet
  help: python main.py --help

Keywords: example, demo, hello, starter

Status: Installed (version 1.0.0)
Location: C:\Users\you\.nex\packages\example.hello-world
```

### Updating Packages

```bash
# Update a specific package
nex update <package-id>

# Update all installed packages
nex update
```

### Removing Packages

```bash
nex remove <package-id>
```

## Configuration

Nex stores data in your home directory:

- **Windows:** `%USERPROFILE%\.nex\`
- **Linux/macOS:** `~/.nex/`

### Directory Structure

```
.nex/
├── packages/           # Installed packages
│   ├── example.hello-world/
│   └── john.image-converter/
├── installed.json      # Tracking file for installed packages
└── config.json         # User configuration (future)
```

## Troubleshooting

### Package not found

```
[ERROR] Package not found (HTTP 404)
```

The package ID may be incorrect. Use `nex search` to find the correct ID.

### Git not installed

```
[ERROR] Failed to clone repository
```

Nex requires Git to be installed and in your PATH. Install Git:
- Windows: https://git-scm.com/download/win
- Linux: `sudo apt-get install git`
- macOS: `brew install git` or `xcode-select --install`

### Runtime not available

If a package requires Python but you don't have it installed:
- Windows: Download from https://python.org
- Linux: `sudo apt-get install python3`
- macOS: `brew install python3`

### Permission denied

On Linux/macOS, you may need to make the downloaded binary executable:
```bash
chmod +x nex
```
