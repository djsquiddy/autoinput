#!/bin/bash
# Autoinput Autocomplete Installation Script for Zsh
# This script copies the Zsh completion script to a directory in fpath.

# Get the absolute path to the directory containing this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ZSH_COMP_FILE="$SCRIPT_DIR/_autoinput"

if [ ! -f "$ZSH_COMP_FILE" ]; then
    echo "Error: Could not find _autoinput completion file at $ZSH_COMP_FILE"
    exit 1
fi

# Detect shell
if [[ "$SHELL" != *"zsh"* ]] && [ -z "$ZSH_VERSION" ]; then
    echo "Warning: Current shell is not Zsh. This script is intended for Zsh completion installation."
fi

# Determine destination directory
# We use ~/.zsh/completion as a safe, user-level directory.
DEST_DIR="$HOME/.zsh/completion"

if [ ! -d "$DEST_DIR" ]; then
    echo "Creating directory $DEST_DIR..."
    mkdir -p "$DEST_DIR"
fi

echo "Copying completion script to $DEST_DIR..."
cp "$ZSH_COMP_FILE" "$DEST_DIR/"

if [ $? -eq 0 ]; then
    echo "Success! The completion script has been copied to $DEST_DIR."
    
    # Check if DEST_DIR is in fpath
    IN_FPATH=false
    if [ -n "$ZSH_VERSION" ]; then
        for p in "${fpath[@]}"; do
            if [[ "$p" == "$DEST_DIR" ]]; then
                IN_FPATH=true
                break
            fi
        done
    fi
    
    if [ "$IN_FPATH" = false ]; then
        echo ""
        echo "Important: Ensure that $DEST_DIR is in your Zsh fpath."
        echo "Add these lines to your ~/.zshrc if they are not already there:"
        echo ""
        echo "    fpath=($DEST_DIR \$fpath)"
        echo "    autoload -Uz compinit && compinit"
        echo ""
    else
        echo "Your fpath already includes $DEST_DIR. You may need to run 'compinit' or restart your shell to apply changes."
    fi
else
    echo "Error: Failed to copy completion script."
    exit 1
fi
