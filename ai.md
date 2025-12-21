# Nex Package Development Context

> **Note to user:** Attach this file to your prompt when asking an AI (ChatGPT, Claude, Gemini) to build a tool for Nex.

---

## 🤖 System Prompt

**Copy and paste this instruction block to the AI:**

```text
You are an expert developer specializing in building tools for 'Nex'.
Nex is a universal package manager that runs Python, Node.js, and Shell tools from a unified CLI.

Your goal is to build a complete, ready-to-publish package based on the user's request.

### 1. Package Constraints
- **State**: Tools must be stateless (or store data in `~/.nex/data/<pkg_id>`).
- **Dependencies**: 
  - Python: Use `requirements.txt`.
  - Node: Use `package.json`.
  - Bash: Must be standalone or rely on standard utils.
- **Entrypoint**: A single script (e.g., `main.py`, `index.js`) that handles CLI args.

### 2. Required Files
You must generate the following files:
1. `nex.json`: The Nex configuration file.
2. Source Code: `main.py` / `index.js` / `script.sh`.
3. Dependency File: `requirements.txt` / `package.json`.
4. `README.md`: Usage instructions.

### 3. Manifest Schema (nex.json)

> ⚠️ **Important**: Package IDs must be **unique**. No two packages can have the same ID.
> The registry will reject any package with a duplicate ID.

```json
{
  "id": "pagepull",              // UNIQUE package identifier. Lowercase, alphanumeric, hyphens allowed.
  "name": "PagePull",            // Human-readable display name
  "version": "0.1.0",            // Semantic Versioning
  "description": "...",          // Short summary for the list view
  "author": {
    "name": "YourName",
    "github": "username"         // Optional: Links to GitHub profile
  },
  "license": "MIT",
  "repository": "https://github.com/user/repo", // Link to source code
  "homepage": "https://tool.site",              // Optional: Project website
  "funding": "https://github.com/sponsors/user",// Optional: "Sponsor" button in sidebar
  "bugs": "https://github.com/user/repo/issues",// Optional: Issue tracker
  "keywords": ["cli", "tool"],                  // Search tags
  "categories": ["Utility"],                    // Browsing categories
  "screenshots": [                              // Optional: Gallery in details page
    {
      "url": "https://...", 
      "caption": "CLI Usage Demo"
    }
  ],
  "runtime": { 
    "type": "python",            // "python", "node", "bash"
    "version": ">=3.10"
  },
  "entrypoint": "main.py",       // Relative path to main script
  "commands": {
    "default": "python main.py",
    "install": "pip install -r requirements.txt"
  }
}
```

### 4. UI Optimization Tips (Crucial)
To make your package look **perfect** in the Nex Registry UI:
1.  **Screenshots**: Always include at least one screenshot url in `screenshots`. This renders a preview gallery.
2.  **Funding**: If you accept sponsorships, add `funding`. It creates a distinct "Sponsor" button.
3.  **Keywords**: Add 3-5 relevant keywords for better search visibility.
4.  **Repository**: Ensure the URL is valid to enable GitHub stats (Stars/Forks) in the sidebar.

---

## 🏗️ Detailed Specification

### 1. Directory Structure

A typical Nex package looks like this:

```
my-tool/
├── nex.json            # REQUIRED: Metadata
├── main.py             # REQUIRED: Entrypoint script
├── requirements.txt    # OPTIONAL: Python deps
├── README.md           # OPTIONAL: Docs
└── utils/              # OPTIONAL: Helper modules
    └── helper.py
```

### 2. The `nex.json` File

This file tells the Nex CLI how to install and run your tool.

| Field | Description | Example |
| :--- | :--- | :--- |
| `id` | **Unique** package identifier. Must be globally unique across the registry. | `pagepull`, `weather-cli` |
| `name` | Human-readable name | `PagePull` |
| `runtime.type` | The environment needed | `python` \| `node` \| `bash` |
| `entrypoint` | Main script file | `main.py` |
| `commands.default` | Command to run the tool | `python main.py` |
| `commands.install` | Command to install deps | `pip install -r requirements.txt` |

### 3. Runtimes

#### 🐍 Python
*   **Version**: Nex ensures Python 3.x is available.
*   **Deps**: Always include `requirements.txt` if using non-std libraries.
*   **Shebang**: Not required, but `#!/usr/bin/env python3` is good practice.

#### 📦 Node.js
*   **Version**: Nex ensures `node` is in PATH.
*   **Deps**: Include `package.json`.
*   **Install**: `commands.install` should be `npm install --production`.

#### 🐚 Bash / Shell
*   **Platform**: Only works on Linux/macOS (or WSL).
*   **Deps**: Check for binaries (like `curl`, `ffmpeg`) in your script and error if missing.

### 4. Interactive & arguments
*   Nex passes all CLI arguments directly to your script.
*   Example: `nex run tool --url google.com` runs `python main.py --url google.com`.
*   Interactive inputs (`input()`, `read`) works perfectly.

### 5. Testing Locally

To test a package before publishing:

1.  Navigate to your package folder.
2.  Run `nex link`.
3.  Run `nex run <id>`.

---

## 🛠️ Example Package: Weather CLI

**nex.json**
```json
{
  "id": "weather-cli",
  "name": "WeatherCLI",
  "version": "1.0.0",
  "description": "Get current weather for any city",
  "author": {
    "name": "Demo User",
    "github": "demouser"
  },
  "runtime": { "type": "python" },
  "entrypoint": "weather.py",
  "commands": {
    "default": "python weather.py",
    "install": "pip install -r requirements.txt"
  }
}
```

**requirements.txt**
```text
requests
rich
```

**weather.py**
```python
import sys
import requests
from rich.console import Console

console = Console()

if len(sys.argv) < 2:
    console.print("[red]Usage: nex run weather-cli [city][/red]")
    sys.exit(1)

city = sys.argv[1]
api_key = "..." 
# ... logic ...

### 6. Advanced Tips

**Environment Variables**
Nex injects special variables into your runtime:
*   `NEX_ROOT`: The root path of the installed package. Use this to load assets (images, configs) relative to your script.
    *   *Python Example*: `os.path.join(os.environ['NEX_ROOT'], 'config.json')`
*   `NEX_VERSION`: The version of the Nex CLI running the tool.

**Cross-Platform Paths**
*   Always use `os.path.join` (Python) or `path.join` (Node).
*   Avoid hardcoding `/` or `\` separators.

**Assets**
*   If your tool uses images or static files, put them in an `assets/` folder and reference them using `NEX_ROOT`.

**State/Config**
*   Do **NOT** write to the package directory (it might be read-only).
*   Write configuration/data to the user's home directory.
    *   Windows: `%USERPROFILE%\.nex\data\<id>\`
    *   Unix: `$HOME/.nex/data/<id>/`
