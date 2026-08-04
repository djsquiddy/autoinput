#!/bin/bash
# Autoinput Autocomplete Installation Script
# This script copies the completion scripts for Zsh and Bash.

# Get the absolute path to the directory containing this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ZSH_COMP_FILE="$SCRIPT_DIR/_autoinput"
BASH_COMP_FILE="$SCRIPT_DIR/autoinput_completion.bash"

# Detect shell
CURRENT_SHELL=$(basename "$SHELL")

# 1. Zsh Installation
install_zsh() {
    if [ ! -f "$ZSH_COMP_FILE" ]; then
        echo "Error: Could not find _autoinput completion file at $ZSH_COMP_FILE"
        return 1
    fi

    DEST_DIR="$HOME/.zsh/completion"
    if [ ! -d "$DEST_DIR" ]; then
        echo "Creating directory $DEST_DIR..."
        mkdir -p "$DEST_DIR"
    fi

    echo "Copying Zsh completion script to $DEST_DIR..."
    cp "$ZSH_COMP_FILE" "$DEST_DIR/"
    
    if [ $? -eq 0 ]; then
        echo "Success! Zsh completion script has been copied to $DEST_DIR."
        echo "Important: Ensure that $DEST_DIR is in your Zsh fpath."
        echo "Add these lines to your ~/.zshrc if they are not already there:"
        echo "    fpath=($DEST_DIR \$fpath)"
        echo "    autoload -Uz compinit && compinit"
    else
        echo "Error: Failed to copy Zsh completion script."
        return 1
    fi
}

# 2. Bash Installation
install_bash() {
    if [ ! -f "$BASH_COMP_FILE" ]; then
        echo "Error: Could not find autoinput_completion.bash file at $BASH_COMP_FILE"
        return 1
    fi

    # Check for bash-completion package locations
    BASH_COMP_DEST=""
    if [ -d "/usr/share/bash-completion/completions" ]; then
        BASH_COMP_DEST="/usr/share/bash-completion/completions/autoinput"
    elif [ -d "/etc/bash_completion.d" ]; then
        BASH_COMP_DEST="/etc/bash_completion.d/autoinput"
    else
        # Fallback to home directory
        BASH_COMP_DEST="$HOME/.bash_completion.d/autoinput"
        mkdir -p "$HOME/.bash_completion.d"
    fi

    echo "Installing Bash completion script to $BASH_COMP_DEST..."
    if [ -w "$(dirname "$BASH_COMP_DEST")" ]; then
        cp "$BASH_COMP_FILE" "$BASH_COMP_DEST"
    else
        echo "Permission denied. Trying with sudo..."
        sudo cp "$BASH_COMP_FILE" "$BASH_COMP_DEST"
    fi

    if [ $? -eq 0 ]; then
        echo "Success! Bash completion script has been installed."
        if [[ "$BASH_COMP_DEST" == "$HOME/.bash_completion.d/autoinput" ]]; then
            echo "Note: You may need to source this file in your ~/.bashrc:"
            echo "    [ -f $BASH_COMP_DEST ] && . $BASH_COMP_DEST"
        fi
    else
        echo "Error: Failed to install Bash completion script."
        return 1
    fi
}

# Run installation based on detected or requested shell
if [[ "$CURRENT_SHELL" == *"zsh"* ]] || [[ "$1" == "zsh" ]]; then
    install_zsh
fi

if [[ "$CURRENT_SHELL" == *"bash"* ]] || [[ "$1" == "bash" ]]; then
    install_bash
fi

if [ "$1" == "all" ]; then
    install_zsh
    install_bash
fi

if [ -z "$1" ] && [[ "$CURRENT_SHELL" != *"zsh"* ]] && [[ "$CURRENT_SHELL" != *"bash"* ]]; then
    echo "Usage: $0 [zsh|bash|all]"
    echo "Current shell ($CURRENT_SHELL) not automatically recognized for installation."
fi
