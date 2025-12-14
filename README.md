<div align="center">

```
  ███╗   ██╗███████╗██╗  ██╗
  ████╗  ██║██╔════╝╚██╗██╔╝
  ██╔██╗ ██║█████╗   ╚███╔╝
  ██║╚██╗██║██╔══╝   ██╔██╗
  ██║ ╚████║███████╗██╔╝ ██╗
  ╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝
```

# Nimble Executor (Nex)

**The AI-Native Package Manager for Developer Tools**

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.5.6-green.svg)](https://github.com/devkiraa/nex/releases)
[![Platform](https://img.shields.io/badge/platform-win%20%7C%20linux%20%7C%20macos-lightgrey.svg)]()
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)

[Features](#-features) • [Installation](#-usage) • [For AI Agents](#-for-ai-agents) • [Docs](docs/)

</div>

---

## 🚀 Overview

**Nex** is a next-generation package manager designed for the era of AI-generated tools. Unlike traditional package managers that are bound to a specific language (pip for Python, npm for Node), Nex is **universal**. It allows you to distribute and run tools written in Python, Node.js, Bash, and Binary formats through a single, diverse CLI.

Built for speed in C, Nex handles all the complexity of runtime detection, dependency management (`pip install`, `npm install`), and path resolution, so you can focus on building.

## ✨ Features

| Feature | Description |
| :--- | :--- |
| **🤖 AI-Native** | Specialized system prompts to help LLMs build valid packages instantly. |
| **⚡ Blazing Fast** | Written in C for sub-millisecond startup times. Zero JVM/V8 overhead. |
| **🌍 Universal** | Run Python, Node.js, and Shell scripts with a single `nex run` command. |
| **📦 Zero Config** | Automatically handles virtual environments and dependencies for you. |
| **🔗 Decentralized** | No central database. The registry is just a GitHub repository. |
| **🛠️ Developer First** | Built-in scaffolding (`init`), linking (`link`), and debugging (`doctor`). |

## 📦 Usage

### Installation

**Windows (PowerShell)**
```powershell
iwr https://nex.sh/install.ps1 | iex
```

**Linux / macOS**
```bash
curl -fsSL https://nex.sh/install.sh | bash
```

### Basic Commands

```bash
# 🔍 Search for tools
nex search "youtube downloader"

# 📥 Install a tool (e.g., pagepull)
nex install devkiraa.pagepull

# 🏃 Run it
nex run pagepull --url https://example.com

# ⚡ Create a shortcut (alias)
nex alias pp pagepull
nex run pp --help

# 🏥 Check system health
nex doctor
```

## 🤖 For AI Agents

Building a tool with ChatGPT, Claude, or Gemini? Copy the prompt below to generate a fully compatible Nex package in seconds.

<details open>
<summary><b>📋 Click to copy System Prompt</b></summary>

```markdown
# Nex Package Creator System Prompt

You are an expert developer task to create a package for the Nex package manager.
Nex is a universal CLI tool runner that supports Python, Node.js, and Bash.

## 1. Package Structure
A Nex package resides in a GitHub repository and must contain:
1. `manifest.json`: Metadata and execution instructions.
2. Source Code: The actual script implementation (e.g., main.py).
3. Dependencies: `requirements.txt` (Python) or `package.json` (Node).

## 2. Manifest Schema (manifest.json)
```json
{
  "$schema": "https://raw.githubusercontent.com/devkiraa/nex/main/registry/schema/package.schema.json",
  "id": "username.package-name",
  "name": "package-name",
  "version": "1.0.0",
  "description": "Short description of what the tool does.",
  "author": { "name": "username", "github": "username" },
  "license": "MIT",
  "repository": "https://github.com/username/package-name",
  "runtime": { "type": "python" },  // Options: "python", "node", "bash"
  "entrypoint": "main.py",          // Main script file
  "commands": {
    "default": "python main.py",    // Command to run the tool
    "install": "pip install -r requirements.txt" // Dependency installation cmd
  }
}
```

## 3. Your Task
1. Write the code for the tool requested by the user.
2. Generate the `manifest.json` following the schema above.
3. Generate the dependency file (`requirements.txt`).
4. Provide instructions to initialize using `nex init` and publish.

Now, please build a tool that [INSERT YOUR TOOL IDEA HERE].
```
</details>

## 🛠️ Command Reference

| Command | Description |
| :--- | :--- |
| `nex install <pkg>` | Install a package from the registry |
| `nex run <pkg>` | Run an installed package |
| `nex list [-v]` | List installed packages (verbose mode available) |
| `nex search <query>` | Search the registry for packages |
| `nex remove <pkg>` | Uninstall a package |
| `nex update` | Update all installed packages |
| `nex outdated` | Check for newer versions in the registry |
| `nex doctor` | Diagnose system issues |
| `nex init` | Interactive wizard to create a new package |
| `nex link` | Link current directory to global scope (for dev) |
| `nex lock` | Generate `nex.lock` snapshot |
| `nex config` | Manage CLI configuration |

## 🤝 Contributing

We welcome contributions! Please check [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to set up the development environment.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

Distributed under the MIT License. See `LICENSE` for more information.

---

<div align="center">
  <sub>Built with ❤️ by <a href="https://github.com/devkiraa">DevKiraa</a></sub>
</div>
