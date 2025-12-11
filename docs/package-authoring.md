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
registry/packages/<first-letter>/<author>/<package-name>/manifest.json
```

For a package `john.image-converter`:
```
registry/packages/j/john/image-converter/manifest.json
```

### Manifest Schema

```json
{
  "id": "author.package-name",
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
| `id` | Unique identifier: `author.package-name` |
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
nex run author.package-name          # runs "default"
nex run author.package-name convert  # runs "convert"
```

## Step 3: Submit to Registry

### Fork the Repository

1. Go to https://github.com/devkiraa/nex
2. Click "Fork" to create your own copy

### Add Your Package

1. Clone your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/nex.git
   cd nex
   ```

2. Create your package directory:
   ```bash
   mkdir -p registry/packages/j/john/my-tool
   ```

3. Create your `manifest.json` in that directory

4. Update `registry/index.json` to include your package:
   ```json
   {
     "packages": [
       {
         "id": "john.my-tool",
         "name": "My Tool",
         "version": "1.0.0",
         "description": "Short description",
         "keywords": ["utility"],
         "manifest": "packages/j/john/my-tool/manifest.json"
       }
     ]
   }
   ```

### Submit Pull Request

1. Commit your changes:
   ```bash
   git add .
   git commit -m "Add package: john.my-tool"
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

- Use your GitHub username as the author
- Use lowercase, hyphens for spaces
- Keep it short but descriptive
- Examples: `john.image-resize`, `acme.data-parser`

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
