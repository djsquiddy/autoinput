# autoinput

A versatile C++ utility for automating mouse and keyboard input on Windows and Linux.

### Features

- **Autoclicker**: Simulate mouse clicks (left, right, middle) at specified intervals. Support for keyboard modifiers (e.g., `shift+left`) is included.
- **Auto-keypresser**: Simulate keyboard input.
- **Flexible Triggering**: Start and stop actions using global hotkeys.
- **Focus Management**: Ability to whitelist or blacklist specific applications to prevent automation from running when they are in focus.
- **Delay Randomization**: Support for randomized press and release delays (e.g., `500ms..1s`).
- **Configuration Support**: Load complex action mappings from TOML files. The program automatically looks for a `configs/` directory relative to its executable.
- **Safety First**: Integrated with Microsoft GSL for robust memory management.
- **Cross-platform Support**: Works on Windows (via `SendInput`), Linux X11 (via `XTest`), and Linux Wayland (via `uinput`).

### Requirements

- **OS**: Windows or Linux (X11 and Wayland)
- **Compiler**: C++23 compliant compiler (e.g., MinGW GCC 13+, MSVC 2022, GCC 13+)
- **Build System**: CMake 3.20+
- **Linux Dependencies**: `libX11`, `libXtst` (optional, for X11 support), kernel headers for `uinput`/`evdev` (for Wayland).

### Building

**Windows (PowerShell):**
```powershell
mkdir build
cd build
cmake ..
cmake --build .
```

**Linux:**
```bash
mkdir build
cd build
cmake ..
make
```

### Shell Completion

#### Clink (Windows)

If you use [Clink](https://chrisant996.github.io/clink/) for `cmd.exe` on Windows, you can enable autocompletion by copying the `scripts/autoinput_completion.lua` file into your Clink scripts directory (usually `%LOCALAPPDATA%\clink`).

#### Zsh (Linux/WSL)

To enable autocompletion in Zsh, add the `scripts/` directory to your `$fpath` in your `~/.zshrc` and initialize completion:

```zsh
fpath=(/path/to/autoinput/scripts $fpath)
autoload -Uz compinit
compinit
```

Alternatively, you can copy the `scripts/_autoinput` file to a directory already in your `$fpath` (e.g., `/usr/local/share/zsh/site-functions`).

### Usage

```bash
autoinput [options]
```

#### Command Line Options

- `[type] {click|hold}`: Set the action type (optional, defaults to `click`).
- `[button] {left|right|middle|back|forward}`: Select mouse buttons to automate. Modifiers like `shift+left` or `ctrl+alt+right` are supported.
- `[key] {key}`: Select keyboard keys to automate.
- `-s, --start, --start-key START_KEYS`: Hotkey(s) or mouse buttons (e.g., `back`, `forward`) to start the automation (defaults to `f2`).
- `-e, --end, --end-key END_KEY`: Hotkey or mouse button (e.g., `back`, `forward`) to stop the automation (defaults to `f3`).
- `-a, --app, --application APPLICATION_NAME`: Only listen for inputs when this application is in focus.
- `-B, --blacklist APPLICATION_NAME`: Do not run when this application is in focus.
- `-L, --list-apps`: List all currently running application names and exit.
- `-w, --wait TIME`: Max duration to wait before auto clicking (e.g., `2s`).
- `--press-wait RANGE`: Randomized delay while button/key is pressed (e.g., `100ms..200ms`).
- `--release-wait RANGE`: Randomized delay between actions (e.g., `1s..2s`).
- `-c, --config FILE`: Load settings from a TOML configuration file.
- `-l, --log LEVEL`: Set logging level (debug, info, warning, error).

#### Examples

1.  **Hold left click** when pressing `F2` and stop on `F3`:
    ```bash
    autoinput hold left
    ```

2.  **Hold left click** on `F2` and **hold right click** on `F3`:
    ```bash
    autoinput hold left f2 right f3
    ```

3.  **Click** on `F2` and **hold** on `F4` (both left mouse button):
    ```bash
    autoinput click f2 hold f4
    ```

4.  **Auto left click** every 1 to 2 seconds:
    ```bash
    autoinput left --press-wait 1s..2s
    ```

5.  **Use a combination with modifiers**:
    ```bash
    autoinput click shift+left f2
    ```

6.  **Hold left click but stop if notepad is in focus**:
    ```bash
    autoinput hold left --blacklist notepad.exe
    ```

7.  **Use a configuration file**:
    ```bash
    autoinput -c core-keeper-fishing
    ```

### Configuration Format

Settings can be defined in `.toml` files. The program looks for these in a `configs/` directory located next to the `autoinput` binary:

```toml
[command]
action = 'click'
button = 'right'
start = 'f2'
end = 'f3'
blacklist = ['notepad.exe', 'calculator.exe']

[command.time]
press = '500ms..750ms'
release = '4s..6s'
```
