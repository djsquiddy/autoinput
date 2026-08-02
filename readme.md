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
- **Mutually Exclusive Command Groups**: Declare groups of commands where only one can be active at a time. Starting one command in a group automatically stops any other active command in the same group.
- **Safety First**: Integrated with Microsoft GSL for robust memory management.
- **Desktop Notifications**: Optional desktop status notifications for active/paused state changes.
- **Cross-platform Support**: Works on Windows (via `SendInput`), Linux X11 (via `XTest`), and Linux Wayland (via `uinput`).
- **No Network Dependencies**: No external network connections or dependencies. No telemetry or analytics.

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

You can use the provided scripts to automatically install or update shell completions.

**Windows:**
```cmd
.\scripts\install.cmd
```

**Linux/Zsh (Bash/Zsh):**
```bash
./scripts/install.sh
```

#### Clink (Windows)

If you use [Clink](https://chrisant996.github.io/clink/) for `cmd.exe` on Windows, you can also manually enable autocompletion by copying the `scripts/autoinput_completion.lua` file into your Clink scripts directory (usually `%LOCALAPPDATA%\clink`).

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
- `--status-notification MODE`: Set status notification mode (`off`, `console`, `desktop`, `both`).
- `-S, --save-config NAME`: Save the current active configuration to the user-level configuration directory as `NAME.toml`.
- `--validate-config NAME-OR-PATH`: Validate the specified configuration file and exit.
- `--duplicate-config SOURCE DESTINATION`: Duplicate an existing config to a new user config.
- `--copy-config SOURCE DESTINATION`: Alias for `--duplicate-config`.
- `--force`: Allow overwriting an existing destination configuration.
- `--json`: Output validation results as machine-readable JSON. Only applies to `--validate-config`.

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

### Configuration Validation

You can validate a configuration file without running the autoclicker. This is useful for verifying complex TOML files or for use in automated scripts.

- **Human-readable output**:
  ```bash
  autoinput --validate-config my-config
  ```

- **Machine-readable JSON output**:
  ```bash
  autoinput --validate-config my-config --json
  ```

Example JSON output for a valid configuration:
```json
{
  "valid": true,
  "configPath": "C:\\path\\to\\configs\\my-config.toml",
  "errors": []
}
```

9.  **Duplicate a configuration**:
    ```bash
    autoinput --duplicate-config core-keeper-fishing my-fishing-copy
    ```
    This copies the built-in or user config `core-keeper-fishing` to a new user config named `my-fishing-copy.toml`. Note that the destination is always written to the user config directory and will not overwrite existing configs unless `--force` is used.

10. **Duplicate using alias**:
    ```bash
    autoinput --copy-config old-config new-config
    ```

### Configuration

The application supports loading settings from TOML files.

#### Locations

1. **Built-in**: The `configs/` directory next to the executable.
2. **User**: `${HOME}/.autoinput/` on Linux or `%USERPROFILE%/.autoinput/` on Windows.

The `settings.toml` file in the user directory is automatically loaded and takes precedence over the built-in `settings.toml`.

#### Format

You can define one or more commands using TOML command blocks.

For a single command, you may use a `[command]` table:

```toml
    [command]
    action = "click"
    button = "left"
    start = "f2"
    time = { press = "10ms..50ms", release = "1s..2s" }
```

For multiple commands, use one `[[command]]` block per command:

```toml
    [[command]]
    action = "click"
    button = "left"
    start = "f2"
    time = { press = "10ms..50ms", release = "1s..2s" }

    [[command]]
    action = "hold"
    button = "right"
    start = "f4"
    time = { press = "25ms", release = "500ms" }
```

Each `[[command]]` block is an independent automation command. The next command starts at the next `[[command]]` block.

This format is recommended for multi-command setups because it avoids the ambiguity of positional command-line arguments.

#### Global Settings

Global settings can be placed at the top level of the file:

```toml
    end = "f3"
    application = "MyGame.exe"
    blacklist = ["overlay.exe"]
    appendBlacklist = true
    statusNotificationMode = "both"

    [[command]]
    action = "click"
    button = "left"
    start = "f2"
    time = { press = "10ms..50ms", release = "1s..2s" }

    [[command]]
    action = "hold"
    button = "right"
    start = "f4"
    time = { press = "25ms", release = "500ms" }
```

Supported global settings:

- `end`: Hotkey or mouse button used to stop automation.
- `application`: Only allow automation while this application is focused.
- `blacklist`: Applications where automation should not run.
- `appendBlacklist`: When `true`, append this config's blacklist to the default blacklist. When `false`, replace the default blacklist.

#### Command Settings

Each command can define its own action, target, start trigger, and timing.

```toml
    [[command]]
    name = "left-click"
    exclusiveGroup = "click-mode"
    action = "click"
    button = "left"
    start = "f2"
    time = { press = "100ms", release = "250ms" }
```

Supported command settings:

- `name`: optional command name used for readability/debugging.
- `exclusiveGroup`: optional group name used to make commands mutually exclusive.
- `action`: The automation action. Common values are `"click"` and `"hold"`.
- `button`: Mouse button to automate, such as `"left"`, `"right"`, `"middle"`, `"back"`, or `"forward"`.
- `key`: Keyboard key to automate, such as `"space"`, `"enter"`, or `"a"`.
- `start`: Hotkey or mouse button used to toggle this command.
- `time`: Timing configuration for the command.

#### Mutually Exclusive Command Groups

You can declare that some commands cannot run at the same time by assigning them to the same `exclusiveGroup`. This is useful for cases where multiple automation modes might conflict if run simultaneously.

Behavior:
- Commands with the same non-empty `exclusiveGroup` are mutually exclusive.
- Starting one command in an exclusive group automatically stops any other active command in the same group.
- Commands without an `exclusiveGroup` are independent and can run alongside any other command.
- Commands in different `exclusiveGroup` values are independent and can run alongside each other.

Example:

```toml
[[command]]
name = "left-click"
exclusiveGroup = "left-click-mode"
action = "click"
button = "left"
start = "back"
time = { press = "25ms", release = "100ms" }

[[command]]
name = "shift-left-click"
exclusiveGroup = "left-click-mode"
action = "click"
button = "shift+left"
start = "forward"
time = { press = "25ms", release = "100ms" }

[[command]]
name = "space"
action = "click"
key = "space"
start = "f6"
time = { press = "50ms", release = "1s" }
```

In this example:
- `left-click` and `shift-left-click` belong to the same group (`left-click-mode`).
- Starting `left-click` will stop `shift-left-click` if it is active.
- Starting `shift-left-click` will stop `left-click` if it is active.
- `space` has no `exclusiveGroup` and can run at the same time as either click mode.

#### Timing

Command timing is configured with the `time` table:

```toml
    time = { press = "100ms", release = "250ms" }
```

Timing values:

- `press`: How long the button or key is held down.
- `release`: How long to wait between repeated actions.

Ranges are supported:

```toml
    time = { press = "100ms..250ms", release = "1s..2s" }
```

You can specify only one timing value if needed:

```toml
    time = { press = "100ms" }
```

```toml
    time = { release = "1s..2s" }
```

Expanded TOML timing tables are also accepted when loading existing configs:

```toml
    [command.time]
    press = "100ms"
    release = "250ms"
```

However, saved configs prefer the inline style:

```toml
    time = { press = "100ms", release = "250ms" }
```

#### Full Example
```toml
    end = "f3"
    application = "Core Keeper"
    blacklist = ["Discord.exe", "Steam Overlay"]
    appendBlacklist = true

    [[command]]
    action = "click"
    button = "left"
    start = "f2"
    time = { press = "10ms..50ms", release = "1s..2s" }

    [[command]]
    action = "hold"
    button = "right"
    start = "f4"
    time = { press = "25ms", release = "500ms" }

    [[command]]
    action = "click"
    key = "space"
    start = "f6"
    time = { press = "50ms", release = "750ms..1s" }
```
Run the config with:

```bash
    autoinput --config my-config
```

or:

```bash
    autoinput -c my-config
```
