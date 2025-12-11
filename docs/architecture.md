# Architecture Overview

This document describes the technical architecture of Nex.

## System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                          User's Machine                             │
│                                                                     │
│  ┌─────────────┐    ┌──────────────────────────────────────────┐  │
│  │ nex         │───▶│  ~/.nex/                                    │  │
│  │ CLI (.exe)  │    │  ├── packages/                            │  │
│  └─────────────┘    │  │   ├── author.package-1/                │  │
│         │           │  │   └── author.package-2/                │  │
│         │           │  └── installed.json                       │  │
│         ▼           └──────────────────────────────────────────┘  │
└─────────┼───────────────────────────────────────────────────────────┘
          │
          │ HTTPS
          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                           GitHub                                     │
│                                                                     │
│  ┌────────────────────────────────────────────────────────────────┐│
│  │ devkiraa/nex (Registry Repository)                             ││
│  │                                                                 ││
│  │  registry/                                                      ││
│  │  ├── index.json          ◄─── Package index                    ││
│  │  └── packages/                                                  ││
│  │      └── a/author/pkg/                                          ││
│  │          └── manifest.json  ◄─── Package metadata              ││
│  └────────────────────────────────────────────────────────────────┘│
│                                                                     │
│  ┌────────────────────────────────────────────────────────────────┐│
│  │ author/package-repo (Tool Repository)                          ││
│  │                                                                 ││
│  │  main.py                  ◄─── Actual tool code                ││
│  │  requirements.txt                                               ││
│  └────────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────┘
```

## Components

### CLI (C Executable)

The CLI is a single compiled executable written in C. It handles:

- Command parsing and routing
- HTTP requests to GitHub raw URLs
- JSON parsing with cJSON library
- Git operations (via system calls)
- Local file management

**Key Dependencies:**
- `libcurl` - HTTP/HTTPS requests
- `cJSON` - JSON parsing (vendored)

**Source Structure:**
```
cli/
├── src/
│   ├── main.c              # Entry point, argument parsing
│   ├── commands/           # Command implementations
│   │   ├── install.c
│   │   ├── run.c
│   │   ├── update.c
│   │   ├── remove.c
│   │   ├── list.c
│   │   ├── search.c
│   │   └── info.c
│   ├── http/
│   │   └── client.c        # HTTP client using libcurl
│   ├── package/
│   │   └── manager.c       # Package operations
│   ├── config/
│   │   └── config.c        # Local configuration
│   └── utils/
│       └── utils.c         # Utilities
├── include/
│   └── nex.h               # Header file
├── deps/
│   └── cJSON/              # Vendored JSON library
└── CMakeLists.txt
```

### Registry

The registry is a directory structure in the main repository containing:

1. **index.json** - Master list of all packages
2. **Package manifests** - Individual JSON files describing each package

**Index Structure:**
```json
{
  "version": "1.0",
  "updated": "2025-12-11T00:00:00Z",
  "packages": [
    {
      "id": "author.package-name",
      "name": "Display Name",
      "version": "1.0.0",
      "description": "Short description",
      "keywords": ["tag1", "tag2"],
      "manifest": "packages/a/author/package-name/manifest.json"
    }
  ]
}
```

**Manifest Structure:**
```json
{
  "id": "author.package-name",
  "name": "Display Name",
  "version": "1.0.0",
  "description": "Full description",
  "repository": "https://github.com/author/package-repo",
  "runtime": {
    "type": "python",
    "version": ">=3.8"
  },
  "entrypoint": "main.py",
  "commands": {
    "default": "python main.py"
  }
}
```

### Frontend (Astro)

Static site generated at build time from registry data.

**Features:**
- Package browsing and search
- Package detail pages
- Documentation
- Installation instructions

**Build Process:**
1. Fetch `registry/index.json` from GitHub
2. Generate static pages for each package
3. Deploy to GitHub Pages

## Data Flow

### Install Command

```
nex install author.pkg
        │
        ▼
┌───────────────────────┐
│ 1. Build manifest URL │
│    from package ID    │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│ 2. GET manifest from  │
│    raw.githubusercontent│
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│ 3. Parse JSON to get  │
│    repository URL     │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│ 4. git clone --depth 1│
│    to ~/.nex/         │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│ 5. Run install command│
│    if defined         │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│ 6. Update installed.json│
└───────────────────────┘
```

### Run Command

```
nex run author.pkg cmd
        │
        ▼
┌───────────────────────┐
│ 1. Check if installed │
│    (if not, install)  │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│ 2. Read local manifest│
│    from package dir   │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│ 3. Find command in    │
│    manifest commands  │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│ 4. cd to package dir  │
│    && execute command │
└───────────────────────┘
```

## Security Considerations

### Trust Model

- Package authors are trusted when their PR is merged
- All packages are from public GitHub repositories
- Users should review packages before installing

### Sandboxing

Currently, packages run with full user permissions. Future versions may include:
- Runtime restrictions
- Permission prompts
- Sandboxed execution

### Verification

- Packages are validated against JSON schema on submission
- Manifest IDs must match index entries
- Repository URLs must be valid GitHub URLs

## Extensibility

### Adding New Runtimes

1. Add enum value in `nex.h`
2. Update `runtime_from_string()` and `runtime_to_string()` in `utils.c`
3. Add execution logic in `package_execute()` in `manager.c`

### Adding New Commands

1. Create new file in `cli/src/commands/`
2. Add function declaration in `nex.h`
3. Add dispatch case in `main.c`
4. Update CMakeLists.txt

## Build System

### CMake

The project uses CMake for cross-platform builds:

```cmake
cmake_minimum_required(VERSION 3.10)
project(nex)

find_package(CURL REQUIRED)

add_executable(nex ...)
target_link_libraries(nex ${CURL_LIBRARIES})
```

### CI/CD

GitHub Actions handles:
- Building binaries for Windows, Linux, macOS
- Validating registry on PRs
- Deploying frontend to GitHub Pages
- Creating releases with attached binaries
