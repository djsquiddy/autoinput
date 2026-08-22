# autoinput

A versatile C++ utility for automating mouse and keyboard input on Windows and Linux.

### Features

- Mouse and keyboard automation.
- Click and hold actions.
- Keyboard and mouse hotkeys for start/toggle/stop.
- Modifier/key-combo support such as `shift+left` and `ctrl+v`.
- Application focus allowlist/blacklist behavior.
- Fixed and randomized timing ranges.
- TOML configuration loading.
- Multi-command configurations.
- Mutually exclusive command groups via `exclusiveGroup`.
- Sequence recording and playback.
- Configuration listing, validation, duplication/copying, and saving.
- JSON output where supported, especially validation.
- Console and desktop status notifications.
- Runtime service mode for frontend/integration use.
- Optional graphical UI frontend.
- Optional Windows system tray frontend.
- Windows, Linux X11, and Linux Wayland backend support.
- Configurable build options for optional components.
- No telemetry / no analytics.

## Responsible Use Disclaimer

> AutoInput is intended for legitimate automation use cases such as accessibility, productivity, software testing, and user-authorized workflows. It is not designed or intended for cheating, gaining unfair advantages in games, bypassing anti-cheat systems, violating terms of service, or automating applications without permission.
>
> AutoInput uses standard operating-system input APIs and does not attempt to hide itself, evade detection, inject into games, modify game memory, defeat anti-cheat systems, or bypass application protections. Users are solely responsible for ensuring their use of AutoInput complies with applicable laws, policies, and the rules of any software or game they interact with.
>
> Online, multiplayer, competitive, or anti-cheat-protected games may prohibit macros, auto-clickers, or synthetic input. Do not use AutoInput with any game or application unless automation is explicitly allowed.
>
> For more information, see the [Responsible Use Policy](docs/responsible-use.md).

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

### Building with CMake Presets

If you are using CMake 3.19 or later, you can use CMake Presets:

```bash
# List available presets
cmake --list-presets

# Configure and build using a preset (e.g., release)
cmake --preset release
cmake --build --preset release

# To build everything (CLI, tests, tray, UI)
cmake --preset all
cmake --build --preset all
```

### Build Options

You can customize the build by passing the following options to `cmake`:

- `AUTOINPUT_BUILD_TESTS`: Build unit and integration tests (defaults to `OFF`).
- `AUTOINPUT_BUILD_TRAY`: Build the optional system tray frontend for Windows (defaults to `OFF`).
- `AUTOINPUT_BUILD_UI`: Build the optional graphical UI frontend (defaults to `OFF`).
- `ENABLE_KEYBOARD_HOOK`: Enable low-level keyboard hook support for global hotkeys (defaults to `ON`).
- `ENABLE_MOUSE_HOOK`: Enable low-level mouse hook support for global hotkeys (defaults to `ON`).
- `ENABLE_FAKE_HOOK`: Use a dummy hook implementation for development or restricted environments (defaults to `OFF`).

### Code Formatting

The project uses `clang-format` for C++ source formatting. Formatting rules are defined by the root-level `.clang-format` file.

Developers should have `clang-format` installed and can verify it with:

```bash
clang-format --version
```

If `clang-format` is available when CMake configures the project, the build provides two helper targets:

```bash
cmake --build build --target format
```

This formats project C++ source/header files under `src/` and `tests/`.

```bash
cmake --build build --target format-check
```

This checks formatting without modifying files, which is useful before committing or in CI.

### Static Analysis

The project uses `.clang-tidy` for optional static analysis. 

Developers should have `clang-tidy` installed and can verify it with:

```bash
clang-tidy --version
```

If `clang-tidy` is available when CMake configures the project, you can use the build directory compile database:

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Run the manual target:

```bash
cmake --build build --target clang-tidy-check
```

Alternatively, you can enable `clang-tidy` during normal builds. This is opt-in and is not enabled by default:

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DAUTOINPUT_ENABLE_CLANG_TIDY=ON
cmake --build build
```

For single-file formatting:

```bash
clang-format -i src/autoinput/autoinput.cpp
```

Most editors/IDEs can detect the root `.clang-format` file automatically. In CLion, ClangFormat can be enabled under:

`Settings | Editor | Code Style | C/C++`

### Shell Completion

You can use the provided scripts to automatically install or update shell completions.

**Windows:**
```cmd
.\scripts\autocomplete\install.cmd
```

**Linux/macOS (Bash/Zsh):**
```bash
./scripts/autocomplete/install.sh
```

#### Clink (Windows)

If you use [Clink](https://chrisant996.github.io/clink/) for `cmd.exe` on Windows, you can also manually enable autocompletion by copying the `scripts/autocomplete/autoinput_completion.lua` file into your Clink scripts directory (usually `%LOCALAPPDATA%\clink`).

#### Zsh (Linux/WSL)

To enable autocompletion in Zsh, add the `scripts/autocomplete` directory to your `$fpath` in your `~/.zshrc` and initialize completion:

```zsh
fpath=(/path/to/autoinput/scripts/autocomplete $fpath)
autoload -Uz compinit
compinit
```

Alternatively, you can copy the `scripts/autocomplete/_autoinput` file to a directory already in your `$fpath` (e.g., `/usr/local/share/zsh/site-functions`).

#### CLI Help & Autocomplete Metadata

`resources/cli/help.toml` is the single canonical source of truth for autoinput's CLI help text (usage, descriptions, options, examples, notes) and shell autocomplete metadata (completion lists for log levels, action types, mouse buttons, notification modes, etc.). It is consumed by two independent tools:

- **`scripts/gen_cli_help.py`**: Generates the C++ `cliHelpMetadata.h`/`.cpp` files (via `scripts/cli_help.py`, the shared TOML loader/validator) that the CLI commands render `--help` output from. This runs automatically at CMake configure/build time; you normally don't need to invoke it manually.
- **`scripts/update_autocomplete.py`**: Generates the Zsh/Bash/Clink-Lua completion scripts in `scripts/autocomplete/` directly from the TOML metadata.

You can regenerate or verify either of these manually:

```bash
# Regenerate the C++ CLI help metadata from resources/cli/help.toml
python scripts/gen_cli_help.py

# Verify the generated C++ metadata is up-to-date (CI check mode)
python scripts/gen_cli_help.py --check

# Regenerate autocomplete scripts (reads resources/cli/help.toml by default)
python scripts/update_autocomplete.py

# Verify completion scripts are up-to-date (CI check mode)
python scripts/update_autocomplete.py --check

# Force generating/checking every target (Zsh, Bash, and Clink/Lua) regardless of platform
python scripts/update_autocomplete.py --check --all

# Explicitly select one or more targets
python scripts/update_autocomplete.py --shell zsh --shell bash

# Fall back to extracting metadata from a built binary's --help output instead of the TOML file
python scripts/update_autocomplete.py --source binary --binary build/debug/bin/autoinput.exe
```

By default, `update_autocomplete.py` only regenerates the completion script(s) relevant to your platform: on Windows it generates the Clink/Lua script only; on Linux/macOS it inspects `$SHELL` and generates the matching Zsh or Bash script (or both, if `$SHELL` is unset/unrecognized). Use `--all` to force every target, or `--shell {zsh,bash,lua,all}` (repeatable) to select specific targets explicitly.

The build script (`python scripts/build.py`) automatically runs `update_autocomplete.py` upon completing a build, using this same platform-aware default so a build on a given OS only refreshes that OS's own completion script(s).

### Usage

```bash
autoinput [global options] <command> [options]
```

#### Global Options

- `-h, --help`: Show help. Can be used after a command for command-specific help (e.g., `autoinput help run` or `autoinput run --help`).
- `-l, --log LEVEL`: Set logging level (`debug`, `info`, `warning`, `error`).
- `--json`: Output results as machine-readable JSON. Only applies to specific commands like `config validate`.

#### Commands

- **run**: Run input automation from command options or a TOML configuration.
- **record**: Record input events and save them as a replayable configuration.
- **serve**: Start the runtime protocol service for frontend or integration use.
- **config**: Manage and validate autoinput configuration files.
- **apps**: List all currently running application names.
- **help**: Show help for autoinput commands.

### The `run` Command

Used to start automation. You can specify actions via command-line flags or by loading a configuration file.

#### Options

- `-c, --config NAME_OR_PATH`: Load a TOML configuration.
- `-t, --type {click|hold}`: Set the action type (defaults to `click`).
- `-b, --button {left|right|middle|back|forward}`: Select mouse buttons to automate. Modifiers like `shift+left` are supported.
- `-k, --key {key}`: Select keyboard keys to automate.
- `-s, --start KEY`: Hotkey or mouse button (e.g., `back`, `forward`) to start/toggle the automation (defaults to `f2`).
- `-e, --end KEY`: Hotkey or mouse button to stop all automation (defaults to `f3`).
- `-a, --app APPLICATION`: Only run while this application is in focus.
- `-B, --blacklist APPLICATION`: Do not run while this application is in focus.
- `-w, --wait, --press-wait RANGE`: Randomized delay while button/key is pressed (e.g., `10ms..50ms`).
- `--release-wait RANGE`: Randomized delay between actions (e.g., `1s..2s`).
- `--status-notification MODE`: Set status notification mode (`off`, `console`, `desktop`, `both`).
- `-S, --save-config NAME`: Save the current active configuration to the user-level configuration directory as `NAME.toml`.

#### Examples

1.  **Hold left click** when pressing `F2` and stop on `F3`:
    ```bash
    autoinput run --type hold --button left
    ```

2.  **Hold left click** on `F2` and **hold right click** on `F3`:
    ```bash
    autoinput run --type hold --button left --start f2 --button right --start f3
    ```

3.  **Click** on `F2` and **hold** on `F4` (both left mouse button):
    ```bash
    autoinput run --type click --button left --start f2 --type hold --button left --start f4
    ```

4.  **Auto left click** every 1 to 2 seconds:
    ```bash
    autoinput run --button left --release-wait 1s..2s
    ```

5.  **Use a combination with modifiers**:
    ```bash
    autoinput run --button shift+left --start f2
    ```

6.  **Hold left click but stop if notepad is in focus**:
    ```bash
    autoinput run --type hold --button left --blacklist notepad.exe
    ```

7.  **Use a configuration file**:
    ```bash
    autoinput run --config core-keeper-fishing
    ```

8.  **Save current setup to a user configuration**:
    ```bash
    autoinput run --type hold --button left --start f2 --press-wait 100ms..200ms --save-config my-setup
    ```

### The `record` Command

Used to record a new input sequence and save it as a TOML configuration.

#### Options

- `NAME`: The name of the recording (saved as `NAME.toml`).
- `--start KEY`: Key that starts the recording (defaults to `f8`).
- `--end KEY`: Key that stops the recording (defaults to `f9`).
- `--play-start KEY`: Key that will be used to play back the recorded sequence (defaults to `f6`).
- `--mouse-moves`: Enable recording of mouse movement events.
- `--mouse-sample TIME`: Sampling rate for mouse movement recording (defaults to `25ms`).
- `--force`: Allow overwriting an existing configuration.

#### Examples

1.  **Record a macro** named 'my-macro' starting with `F8` and stopping with `F9`:
    ```bash
    autoinput record my-macro --start f8 --end f9
    ```

2.  **Record mouse moves** with a specific sampling rate:
    ```bash
    autoinput record my-macro --mouse-moves --mouse-sample 50ms
    ```

Once saved, you can play it back using the `run` command:
```bash
autoinput run --config my-macro
```
(By default, the sequence will play when you press `F6`, and can be stopped by the global end key `F3`).

### The `serve` Command

Used to run the runtime protocol service over standard input/output. This is designed for graphical frontends or integration clients that need to control the automation runtime programmatically.

#### Options

- `--stdio`: Use standard input and output for the service protocol (required).

#### Example

```bash
autoinput serve --stdio
```

### The `config` Command

Used to manage, list, and validate configurations.

#### Subcommands

- **list**: List all available built-in and user configurations.
- **validate NAME_OR_PATH**: Validate a configuration file.
- **duplicate SOURCE DESTINATION**: Duplicate an existing config to a new user config.
- **copy**: Alias for `duplicate`.
- **path NAME_OR_PATH**: Print the resolved path to a configuration file.

#### Examples

1.  **List available configurations**:
    ```bash
    autoinput config list
    ```

2.  **Validate a configuration file**:
    ```bash
    autoinput config validate my-config
    ```

3.  **Validate with JSON output**:
    ```bash
    autoinput config validate my-config --json
    ```

4.  **Duplicate a configuration**:
    ```bash
    autoinput config duplicate core-keeper-fishing my-fishing-copy
    ```
    Note that the destination is always written to the user config directory and will not overwrite existing configs unless `--force` is used.

5.  **Get the path to a configuration**:
    ```bash
    autoinput config path my-config
    ```

### The `apps` Command

Used to list all currently running application names. This is helpful for finding the correct names to use with `--app` or `--blacklist`.

```bash
autoinput apps list
```

### System Tray Frontend (Windows only)

`autoinput` includes an optional system tray frontend that allows you to manage automation without using the command line.

#### Features
- Start and stop automation from the system tray.
- Select from available configurations in a submenu.
- Open the user configuration directory directly.
- Real-time status indication via the tray icon and tooltip.
- Desktop notifications for errors and status changes.

#### Building with Tray Support
Tray support is optional and can be enabled during the CMake configuration step:

```powershell
cmake -DAUTOINPUT_BUILD_TRAY=ON ..
cmake --build .
```

This will produce an additional executable named `autoinput_tray` (or `autoinput_tray.exe` on Windows).

#### Running the Tray App
Simply run the `autoinput_tray` executable. It will appear in your system tray (notification area). Right-click the icon to access the menu.

The tray app discovers configurations from the same locations as the CLI:
- Built-in configurations in the `configs/` directory relative to the executable.
- User-level configurations in `%USERPROFILE%/.autoinput/`.

### Graphical UI Frontend

The graphical UI provides visual management of automation, configurations, runtime state, logs, settings, and diagnostics. It is built with Dear ImGui and provides a more comprehensive interface than the system tray application.

#### Features

- Main dashboard/window hub.
- Config manager and config editor.
- Settings editor.
- Command runner.
- Command palette.
- Sequence editor.
- Sequence recorder.
- Runtime dashboard.
- Advanced runtime controls with start/stop/pause/resume.
- Hotkey manager.
- Validation report viewer.
- Log viewer.
- Import/export tools.
- Backup/restore tools.
- Backend diagnostics.
- Application picker.
- Notification tester.
- Setup wizard.
- About window.

#### Building with UI Support

UI support is optional and can be enabled during the CMake configuration step:

```bash
cmake -DAUTOINPUT_BUILD_UI=ON ..
cmake --build .
```

This will produce an additional executable named `autoinput-ui` (or `autoinput-ui.exe` on Windows) when the required dependencies are available.

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

For recorded macros, use `[[sequence]]` blocks:

```toml
    [[sequence]]
    name = "my-macro"
    start = "f6"
    repeat = false

    events = [
      { type = "mouse_move", x = 1000, y = 500, delay = "0ms" },
      { type = "mouse_down", button = "left", x = 1000, y = 500, delay = "120ms" },
      { type = "mouse_up", button = "left", x = 1000, y = 500, delay = "80ms" },
      { type = "key_down", key = "space", delay = "400ms" },
      { type = "key_up", key = "space", delay = "40ms" }
    ]
```

This format is recommended for multi-command setups because it avoids the ambiguity of positional command-line arguments.

#### Global Settings

Global settings can be placed at the top level of the file:

```toml
    end = "f3"
    application = "MyGame.exe"
    blacklist = ["overlay.exe"]
    statusNotificationMode = "both"
    logLevel = "info"

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
- `blacklist`: Applications where automation should not run. If a user settings file defines `blacklist`, it replaces the built-in blacklist.
- `statusNotificationMode`: Set status notification mode (`off`, `console`, `desktop`, `both`).
- `logLevel`: Set logging level (`debug`, `info`, `warning`, `error`).

#### Command Settings

Each command can define its own action, target, start trigger, control bindings, and timing.

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

- `name`: optional command name used for readability/debugging and targeted control actions.
- `exclusiveGroup`: optional group name used to make commands mutually exclusive.
- `action`: The automation action. Common values are `"click"` and `"hold"`.
- `button`: Mouse button to automate, such as `"left"`, `"right"`, `"middle"`, `"back"`, or `"forward"`.
- `key`: Keyboard key to automate, such as `"space"`, `"enter"`, or `"a"`.
- `start`: Hotkey or mouse button used to start/toggle this command (legacy mapping for `toggle`).
- `controls`: List of flexible control bindings for this command (see below).
- `time`: Timing configuration for the command.

#### Flexible Command Control Bindings

Commands support separate configurable control inputs for granular runtime control:

- `start`: Starts the command if not active (or unpauses if paused).
- `toggle`: Starts the command if inactive, stops it if active.
- `stop` / `cancel`: Stops/cancels only the targeted command and releases any pressed keys/buttons. Does **not** exit the application or runtime listener.
- `pause`: Pauses the command execution and releases pressed keys/buttons while keeping the command active.
- `resume`: Resumes the paused command (subject to application blacklist / target checks).
- `toggle-pause`: Toggles between paused and running states for the command.
- `stop-all`: Stops all currently active commands across the application without terminating the runtime.
- `exit`: Stops all commands and exits the application/runtime (equivalent to the global end key).

##### Control Action Differences

| Action | Target Scope | Releases Pressed Input | Exits Application? |
|---|---|---|---|
| `cancel` / `stop` | Targeted command only | Yes | No (App stays running) |
| `stop-all` | All active commands | Yes | No (App stays running) |
| `exit` | All active commands | Yes | Yes (Shuts down runtime) |

##### Synthetic Input Behavior
Synthetic/generated input injected by AutoInput or other automated tools is ignored for control bindings by default to prevent self-triggering loops.

##### Backward Compatibility
Legacy `start` keys or buttons are mapped to `ControlAction::Toggle`, and the global `end` key is mapped to `ControlAction::Exit`, preserving existing configurations and behavior.

##### Example: Mouse Back Start / Mouse Right Cancel

A command that starts left auto-clicking with Mouse Back and cancels with Mouse Right:

```toml
[[command]]
name = "left-clicker"
action = "click"
button = "left"

[[command.controls]]
action = "toggle"
input = "mouse.back"

[[command.controls]]
action = "cancel"
input = "mouse.right"
```

You can also run this directly via the CLI:

```bash
autoinput run --name left-clicker --button left --type click --control toggle:mouse.back --control cancel:mouse.right
```

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
    logLevel = "info"

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
autoinput run --config my-config
```

or:

```bash
autoinput run -c my-config
```

### Testing

If `AUTOINPUT_BUILD_TESTS` is enabled, you can run the tests using `ctest` from the build directory:

```bash
cd build
ctest --output-on-failure
```

Alternatively, you can run the test executable directly:

```bash
./bin/autoinput-tests
```

### Scripts

The `scripts/` directory contains helper scripts for building, testing, maintenance, and shell completion:

- **`build.py` / `build.cmd` / `build.sh`**: Cross-platform Python build runner supporting targets (`ui`, `tray`, `tests`, `all`), build types (`Release`, `Debug`, `RelWithDebInfo`, `MinSizeRel`), clean rebuild flags (`-c, --clean`), and CMake presets (`-p, --preset <name>`, `--list-presets`).
- **`run.cmd`**: Windows helper script to build and launch the application.
- **`appendPath.cmd`**: Windows helper script to temporarily append the build output directory to `PATH`.
- **`gen_localization_ids.py`**: Generates C++ constexpr localization IDs and lookup helper functions from `resources/localization/en-US.toml`.
- **`audit_localization.py`**: Audits C++ source code and `en-US.toml` for missing and unused localization keys.
- **`cli_help.py`**: Shared loader/validator (dataclasses + `load_cli_help_metadata()`) for the canonical CLI metadata in `resources/cli/help.toml`.
- **`gen_cli_help.py`**: Generates the C++ CLI help metadata (`cliHelpMetadata.h`/`.cpp`) consumed by the CLI's `--help` output, from `resources/cli/help.toml`.
- **`test_cli_help.py`**: Plain-assert validation tests for `cli_help.py` (run with `python scripts/test_cli_help.py`).
- **`update_autocomplete.py`**: Updates shell completion scripts for Zsh, Bash, and Clink/Lua from `resources/cli/help.toml`.
- **`autocomplete/`**: Shell completion scripts and installers for Bash, Zsh, and Clink.

Usage examples:
```bash
# Build all components in Release mode
./scripts/build.sh all Release

# Build only CLI and tests in Debug mode
./scripts/build.sh tests Debug

# Build using a specific CMake preset
python scripts/build.py --preset debug-tests
```

### Documentation

Additional documentation is available in the [`docs/`](docs/) directory:
- [Localization Guide](docs/localization.md) — Localization architecture, TOML conventions, and ID generation tooling.
- [Responsible Use Policy](docs/responsible-use.md) — Ethical guidelines and policies for automated inputs.

### License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
