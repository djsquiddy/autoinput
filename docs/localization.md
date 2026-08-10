# UI Localization Documentation

AutoInput UI uses a TOML-based localization system to manage user-facing strings. This allows for easy translation and customization of the user interface without modifying the source code.

## Localization Files

Localization files are stored in the `resources/localization/` directory. Each language has its own TOML file named after its language code (e.g., `en-US.toml`, `de-DE.toml`).

The `en-US.toml` file serves as the primary source of truth and the fallback for all other languages.

## TOML Structure

Localization files use nested TOML tables to group related strings. The keys are accessed using a dot-separated path (e.g., `app.name`).

Example `en-US.toml`:
```toml
[app]
name = "AutoInput"
version = "Version {}"

[buttons]
save = "Save"
cancel = "Cancel"

[modals.saveConfirmation]
title = "Unsaved Changes"
message = "The window '{}' has unsaved changes. Do you want to save them?"
```

## Key Naming Conventions

To keep the localization system organized, we use the following naming conventions for keys:

- `app.*`: Global application strings.
- `windows.*`: Window titles.
- `menus.*`: Menu bar labels and items.
- `buttons.*`: Common button labels.
- `labels.*`: Shared field labels and descriptions.
- `status.*`: Status messages and indicators.
- `modals.*`: Modal dialog titles and messages.
- `actions.*`: UI action labels (used in Command Palette and Main Window).
- `actionCategories.*`: Categories for UI actions.
- `validation.*`: Validation error messages.
- `<featureName>.*`: Feature-specific strings (e.g., `setupWizard.*`, `runtimeDashboard.*`).

## What to Localize

- All user-facing text displayed in windows, menus, and buttons.
- Status messages, error messages, and tooltips intended for the user.
- Table column headers and placeholder text.

## What NOT to Localize

- **Internal IDs**: Window IDs, Action IDs, and widget IDs.
- **Config Keys**: Names of settings or configuration fields in TOML files.
- **Runtime Protocol**: Method names and field names used in communication between the UI and backend.
- **Enum Serialized Values**: Internal string representations of enums.
- **Log Messages**: Developer-facing logs (unless they are also displayed in the UI).
- **User Data**: Values provided by the user (e.g., command names, file paths).

## Fallback Mechanism

The localization system uses a layered loading approach:
1. `en-US.toml` is always loaded first as the base fallback.
2. The user-selected language (e.g., `de-DE.toml`) is loaded over the base.
3. If a key is missing in the selected language, the value from `en-US.toml` is used.
4. If a key is missing in both, the key itself is returned as the localized text.

Missing keys are logged once per session to the application log to help developers identify missing translations.

## Adding a New Language

1. Create a new TOML file in `resources/localization/` named after the language code (e.g., `fr-FR.toml`).
2. Copy the structure from `en-US.toml` and translate the values.
3. The new language will automatically appear in the Language selector in Settings.

## ImGui Stable ID Considerations

ImGui uses the string label of a widget as its unique identifier unless a stable ID is provided. When localizing labels, it is critical to preserve stable IDs to avoid losing widget state (e.g., focus, scroll position).

Use the `##` suffix to provide a stable internal ID that is not visible to the user but remains constant across translations.

```cpp
// Correct: Localized label with stable ID
ImGui::Button(Localization::get().format("{}##SaveButton", loc.text("buttons.save")).c_str());

// Also correct: Use PushID
ImGui::PushID("SaveButton");
if (ImGui::Button(loc.text("buttons.save").data())) { ... }
ImGui::PopID();
```

## Localization API

### `text(key)`
Returns the localized string for the given key. Returns the key itself if not found.
```cpp
auto label = Localization::get().text("buttons.save");
```

### `format(key, args...)`
Returns a formatted localized string using `std::format` syntax.
```cpp
auto msg = Localization::get().format("app.version", "1.0.0");
```

### `textOr(key, fallback)`
Returns the localized string or a specific fallback if not found.
```cpp
auto label = Localization::get().textOr("custom.key", "Default Label");
```

## Maintenance and Auditing

To ensure the localization system remains clean and up-to-date, a developer script is provided to audit the localization keys.

### Audit Script

The `scripts/audit_localization.py` script performs the following checks:
1. **Missing Keys**: Identifies keys used in the source code that are missing from `en-US.toml`.
2. **Unused Keys**: Identifies keys present in `en-US.toml` that are not found in the source code.

**Prerequisites**:
- Python 3.11 or later.

**Usage**:
```bash
python scripts/audit_localization.py
```

The script will exit with a non-zero status if missing keys are detected, making it suitable for CI/CD pipelines. Unused keys are reported as warnings but do not cause the audit to fail, as some keys might be used dynamically or reserved for future use.
