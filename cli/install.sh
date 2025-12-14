#!/bin/bash
set -e

# Detect OS
OS="$(uname -s)"
case "${OS}" in
    Linux*)     PLATFORM=linux;;
    Darwin*)    PLATFORM=macos;;
    *)          echo "Unsupported OS: ${OS}"; exit 1;;
esac

# Detect Arch
ARCH="$(uname -m)"
case "${ARCH}" in
    x86_64)    ARCH=x64;;
    aarch64)   ARCH=arm64;;
    arm64)     ARCH=arm64;;
    *)         echo "Unsupported Architecture: ${ARCH}"; exit 1;;
esac

# Fallback to v1.5.6 if 1.6.0 not released yet, but usually we point to latest
VERSION="v1.6.0" 
BINARY="nex-${PLATFORM}-${ARCH}"
URL="https://github.com/devkiraa/nex/releases/download/${VERSION}/${BINARY}"
INSTALL_DIR="/usr/local/bin"

echo "Installing Nex (${VERSION}) for ${PLATFORM}/${ARCH}..."

if [ ! -w "$INSTALL_DIR" ]; then
    echo "Sudo permission needed to write to ${INSTALL_DIR}"
    SUDO="sudo"
else
    SUDO=""
fi

$SUDO curl -fsSL "${URL}" -o "${INSTALL_DIR}/nex"
$SUDO chmod +x "${INSTALL_DIR}/nex"

echo "✅ Nex installed successfully!"
nex --version
