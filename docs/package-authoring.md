# Package Authoring Guide

Learn how to create and publish packages for Nex.

## Package Structure

A Nex package consists of:

1. **Your tool's code** - Hosted in a public GitHub repository
2. **A manifest file** - Describes your package (in the Nex registry)

## Step 1: Create Your Tool

First, create your tool in a GitHub repository. Here's an example Python tool:

### Repository Structure

```
your-tool/
├── main.py           # Entrypoint
├── requirements.txt  # Dependencies
├── README.md         # Documentation
└── manifest.json     # Package manifest (optional, can be in registry only)
```

### Example: main.py

```python
#!/usr/bin/env python3
"""A simple CLI tool example."""

import sys

def main():
    if len(sys.argv) > 1:
        name = sys.argv[1]
    else:
        name = "World"
    print(f"Hello, {name}!")

if __name__ == "__main__":
    main()
```

### Example: requirements.txt

```
# Add your dependencies here
# click>=8.0.0
# requests>=2.28.0
```

## Step 2: Create the Manifest

The manifest describes your package to Nex.

### Manifest Location

In the Nex registry:
```
registry/packages/<first-letter>/<package-name>/manifest.json
```

For a package `image-converter`:
```
registry/packages/i/image-converter/manifest.json
```

### Manifest Schema

> ⚠️ **Important**: Package IDs must be **unique** across the entire registry.
> The system will reject any package with a duplicate ID.

```json
{
  "id": "my-package",
  "name": "Package Display Name",
  "version": "1.0.0",
  "description": "Short description of what your tool does",
  
  "author": {
    "name": "Your Name",
    "github": "yourusername"
  },
  
  "license": "MIT",
  "repository": "https://github.com/yourusername/your-tool",
  
  "runtime": {
    "type": "python",
    "version": ">=3.8"
  },
  
  "entrypoint": "main.py",
  
  "commands": {
    "default": "python main.py",
    "install": "pip install -r requirements.txt",
    "help": "python main.py --help"
  },
  
  "keywords": ["utility", "automation", "python"],
  
  "platforms": ["windows", "linux", "macos"]
}
```

### Required Fields

| Field | Description |
|-------|-------------|
| `id` | **Unique** package identifier (lowercase, alphanumeric, hyphens allowed) |
| `name` | Human-readable display name |
| `version` | Semantic version (e.g., `1.0.0`) |
| `description` | Brief description (max 500 chars) |
| `repository` | GitHub repository URL |
| `runtime` | Runtime type and version |
| `entrypoint` | Main file to execute |

### Runtime Types

| Type | Description | Command Format |
|------|-------------|----------------|
| `python` | Python scripts | `python <entrypoint>` |
| `node` | Node.js scripts | `node <entrypoint>` |
| `bash` | Bash scripts | `bash <entrypoint>` |
| `powershell` | PowerShell scripts | `powershell -File <entrypoint>` |
| `binary` | Compiled executable | `<entrypoint>` |
| `go` | Go programs | `go run <entrypoint>` |

### Commands

Define named commands users can run:

```json
"commands": {
  "default": "python main.py",
  "install": "pip install -r requirements.txt",
  "convert": "python main.py convert",
  "resize": "python main.py resize",
  "help": "python main.py --help"
}
```

Users run these with:
```bash
nex run my-package          # runs "default"
nex run my-package convert  # runs "convert"
```

## Step 3: Submit to Registry

### Fork the Repository

1. Go to https://github.com/nexhq/nex
2. Click "Fork" to create your own copy

### Add Your Package

1. Clone your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/nex.git
   cd nex
   ```

2. Create your package directory:
   ```bash
   mkdir -p registry/packages/m/my-tool
   ```

3. Create your `manifest.json` in that directory

4. Update `registry/index.json` to include your package:
   ```json
   {
     "packages": [
       {
         "id": "my-tool",
         "name": "My Tool",
         "version": "1.0.0",
         "description": "Short description",
         "keywords": ["utility"],
         "manifest": "packages/m/my-tool/manifest.json"
       }
     ]
   }
   ```

### Submit Pull Request

1. Commit your changes:
   ```bash
   git add .
   git commit -m "Add package: my-tool"
   git push origin main
   ```

2. Go to your fork on GitHub
3. Click "New Pull Request"
4. Describe your package and submit

### Review Process

- Automated checks validate your manifest
- Maintainers review for quality and appropriateness
- Once approved, your package is available to all users!

## Best Practices

### Package ID

- Use lowercase letters, numbers, and hyphens only
- Keep it short but descriptive
- Must be **globally unique** across the registry
- Examples: `image-resize`, `data-parser`, `weather-cli`

### Version Numbering

Follow [Semantic Versioning](https://semver.org/):
- MAJOR.MINOR.PATCH
- `1.0.0` - Initial release
- `1.1.0` - New features (backward compatible)
- `1.1.1` - Bug fixes
- `2.0.0` - Breaking changes

### Documentation

Include a good README.md in your repository:
- What the tool does
- Installation requirements
- Usage examples
- Available commands

### Testing

Before submitting:
1. Clone your tool locally
2. Test all commands work
3. Verify on multiple platforms if possible

## Updating Your Package

To release a new version:

1. Update your tool's code in your GitHub repository
2. Submit a PR to the Nex registry updating:
   - `manifest.json` with new version
   - `index.json` with new version

Users can then run `nex update` to get the latest version.

## The nex.json File

You can optionally add a `nex.json` file to your tool's repository. This file tells Nex exactly how to install and run your tool.

### Why use nex.json?

- **Self-documenting**: Your repo contains all the info Nex needs
- **Version control**: Update run configuration with your code
- **Discoverability**: Nex can auto-detect tools with `nex.json`
- **Simpler registry**: The registry manifest can reference your `nex.json`

### nex.json Structure

```json
{
  "$schema": "https://raw.githubusercontent.com/nexhq/nex/main/registry/schema/nex-package.schema.json",
  "id": "my-tool",
  "name": "My Tool",
  "version": "1.0.0",
  "description": "What the tool does",
  "type": "python",
  
  "python": {
    "version": ">=3.8",
    "entry": "main.py",
    "requirements": "requirements.txt"
  },
  
  "run": {
    "command": "python",
    "args": ["main.py"],
    "passthrough_args": true
  },
  
  "author": {
    "name": "Your Name",
    "github": "yourusername"
  },
  "license": "MIT",
  "repository": "https://github.com/yourusername/my-tool"
}
```

### Package Types

| Type | Description | Requirements |
|------|-------------|--------------|
| `python` | Python script | `python` section with `entry` and optional `requirements` |
| `node` | Node.js app | `node` section with `entry` and optional `package_manager` |
| `binary` | Native executable | `binary` section with platform-specific executables |
| `script` | Shell script | Direct execution |

### Example: Python Tool (PagePull)

```json
{
  "$schema": "https://raw.githubusercontent.com/nexhq/nex/main/registry/schema/nex-package.schema.json",
  "id": "pagepull",
  "name": "PagePull",
  "version": "0.0.1",
  "description": "Pull entire websites for offline viewing",
  "type": "python",
  "python": {
    "version": ">=3.8",
    "entry": "pagepull.py",
    "requirements": "requirements.txt"
  },
  "run": {
    "command": "python",
    "args": ["pagepull.py"],
    "passthrough_args": true
  },
  "author": {
    "name": "devkiraa",
    "github": "devkiraa"
  },
  "license": "MIT",
  "repository": "https://github.com/nexhq/pagepull"
}
```

### Example: Node.js Tool

```json
{
  "id": "my-node-tool",
  "name": "My Node Tool",
  "version": "1.0.0",
  "type": "node",
  "node": {
    "version": ">=18.0.0",
    "entry": "cli.js",
    "package_manager": "npm"
  },
  "run": {
    "command": "node",
    "args": ["cli.js"],
    "passthrough_args": true
  }
}
```

### Example: Binary Tool

```json
{
  "id": "my-binary",
  "name": "My Binary Tool",
  "version": "2.0.0",
  "type": "binary",
  "binary": {
    "windows": "mytool.exe",
    "linux": "mytool",
    "macos": "mytool"
  },
  "run": {
    "command": "${binary}",
    "passthrough_args": true
  }
}
```

### How Nex Uses nex.json

When you run `nex install my-package`:

1. Nex fetches the registry manifest
2. Downloads your tool from GitHub releases
3. Reads `nex.json` (if present) for run configuration
4. Sets up dependencies (pip install, npm install, etc.)

When you run `nex run my-package [args]`:

1. Nex reads the `run` configuration
2. Executes the command with your arguments
3. Passes through all args if `passthrough_args: true`

