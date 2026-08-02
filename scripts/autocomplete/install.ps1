# Autoinput Autocomplete Installation Script for Windows (Clink)
# This script copies the Lua completion script to the Clink profile directory.

$scriptsDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$luaFile = Join-Path $scriptsDir "autoinput_completion.lua"

if (-not (Test-Path $luaFile)) {
    Write-Error "Could not find autoinput_completion.lua at $luaFile"
    exit 1
}

# 1. Clink Installation
$clinkPaths = @(
    Join-Path $env:LOCALAPPDATA "clink"
    Join-Path $env:APPDATA "clink"
)

$installed = $false
foreach ($path in $clinkPaths) {
    if (Test-Path $path) {
        Write-Host "Installing Clink completion script to $path..."
        try {
            Copy-Item $luaFile $path -Force -ErrorAction Stop
            Write-Host "Success! Autocomplete for 'autoinput' is now enabled in Clink."
            $installed = $true
        } catch {
            Write-Warning "Failed to copy to ${path}: $($_.Exception.Message)"
        }
    }
}

if (-not $installed) {
    Write-Warning "Clink directory not found in common locations ($($clinkPaths -join ', '))."
    Write-Host "If you use Clink, please ensure it is installed and copy the script manually:"
    Write-Host "  copy ""$luaFile"" ""%LOCALAPPDATA%\clink\"""
}

# 2. Zsh (WSL/Git Bash)
if (Get-Command "zsh" -ErrorAction SilentlyContinue) {
    Write-Host "`nZsh detected in the environment."
    Write-Host "To install Zsh completions, please run 'scripts/install.sh' from a Zsh-compatible terminal."
}

Write-Host "`nInstallation process finished."
