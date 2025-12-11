# Nex

> **N**imble **Ex**ecutor - A lightweight package manager for developer tools

An open-source package manager for developer tools. Think npm/winget but for Python scripts, CLI utilities, and automation tools.

## Features

- 🚀 **Single executable** - One `.exe` added to PATH, works anywhere
- 📦 **GitHub-backed registry** - No database needed, packages defined in JSON
- 🔧 **Multi-runtime support** - Python, Node.js, PowerShell, Bash, and more
- 🌐 **Web interface** - Browse and search packages at our GitHub Pages site
- 🤝 **Open source** - Anyone can submit packages via Pull Request

## Quick Start

### Installation

Download the latest `nex.exe` from [Releases](https://github.com/devkiraa/nex/releases) and add it to your PATH.

### Usage

```bash
# Install a package
nex install author.package-name

# Run a package command
nex run author.package-name [command] [args...]

# Search for packages
nex search "image converter"

# List installed packages
nex list

# Update a package
nex update author.package-name

# Remove a package
nex remove author.package-name
```

## How It Works

1. **Packages are JSON manifests** in the `registry/packages/` directory
2. **Manifests point to GitHub repos** containing the actual tool code
3. **CLI fetches manifests** from this repo's raw GitHub URLs
4. **Tools are cloned locally** to `~/.nex/packages/`
5. **Execution follows manifest** - runs the specified entrypoint with the configured runtime

## Project Structure

```
nex/
├── cli/           # C source code for the CLI executable
├── frontend/      # Astro-based static website
├── registry/      # Package manifests and index
├── docs/          # Documentation
├── scripts/       # Build and validation scripts
└── .github/       # CI/CD workflows
```

## Contributing

### Submitting a Package

1. Fork this repository
2. Create a manifest in `registry/packages/<first-letter>/<author>/<package-name>/manifest.json`
3. Submit a Pull Request
4. Once merged, your package will be available to all users!

See [Package Authoring Guide](docs/package-authoring.md) for manifest format.

### Contributing to Nex

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup and guidelines.

## License

MIT License - see [LICENSE](LICENSE)

## License

MIT License - see [LICENSE](LICENSE)
