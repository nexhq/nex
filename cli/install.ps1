# Install Nex CLI on Windows
$InstallDir = "$env:USERPROFILE\.nex\bin"
$BinaryUrl = "https://github.com/devkiraa/nex/releases/download/v1.5.6/nex-windows-x64.exe"
$ExePath = "$InstallDir\nex.exe"

Write-Host "Installing Nex to $InstallDir..."

# Create directory
if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
}

# Download
Write-Host "Downloading latest release..."
try {
    Invoke-WebRequest -Uri $BinaryUrl -OutFile $ExePath
} catch {
    Write-Error "Failed to download nex.exe. Check your internet connection."
    exit 1
}

# Add to PATH
$UserPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($UserPath -notlike "*$InstallDir*") {
    Write-Host "Adding to PATH..."
    [Environment]::SetEnvironmentVariable("Path", "$UserPath;$InstallDir", "User")
    Write-Host "✅ Added to PATH. Please restart your terminal/PowerShell window to use 'nex'."
} else {
    Write-Host "✅ Already in PATH."
}

Write-Host "Installation complete! Run 'nex --version' to verify."
