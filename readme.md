# autoinput

A versatile C++ utility for automating mouse and keyboard input on Windows and Linux.

### Features

- **Autoclicker**: Simulate mouse clicks (left, right, middle) at specified intervals. Support for keyboard modifiers (e.g., `shift+left`) is included.
- **Auto-keypresser**: Simulate keyboard input.
- **Flexible Triggering**: Start and stop actions using global hotkeys.
- **Focus Management**: Ability to whitelist or blacklist specific applications to prevent automation from running when they are in focus.
- **Delay Randomization**: Support for randomized press and release delays (e.g., `500ms..1s`).
- **Configuration Support**: Load complex action mappings from TOML files. The program looks for configurations in the `configs/` directory relative to its executable and in the user-level directory (`~/.autoinput/` on Linux, `%USERPROFILE%/.autoinput/` on Windows). User-level configurations in `settings.toml` automatically override built-in defaults.
- **Multi-command Support**: Run multiple independent automation commands simultaneously from a single configuration file.
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
- `-S, --save-config NAME`: Save the current active configuration to the user-level configuration directory as `NAME.toml`.

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

8.  **Save current setup to a user configuration**:
    ```bash
    autoinput hold left f2 --press-wait 100ms..200ms --save-config my-setup
    ```

### Configuration

The application supports loading settings from TOML files.

#### Locations

1.  **Built-in**: The `configs/` directory next to the executable.
2.  **User**: `~/.autoinput/` (Linux) or `%USERPROFILE%/.autoinput/` (Windows).

The `settings.toml` file in the user directory is automatically loaded and takes precedence over the built-in `settings.toml`.

#### Format

You can define one or more commands using `[[command]]` blocks. Global settings like `end`, `application` (whitelist), and `blacklist` can be defined at the top level.

```toml
# Global settings
end = 'f3'
application = 'MyGame.exe'
blacklist = ['overlay.exe']
appendBlacklist = true # Set to false to replace the existing blacklist instead of appending

[[command]]
action = 'click'
button = 'left'
start = 'f2'

[command.time]
press = '10ms..50ms'
release = '1s..2s'

[[command]]
action = 'hold'
button = 'right'
start = 'f4'
```
