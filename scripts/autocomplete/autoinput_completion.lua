-- autoinput clink completion script
-- To use, place this file in a directory that Clink scans for scripts,
-- or add that directory to Clink's lua.path.

local function configs_matcher(word)
    local configs = {}
    local paths = {"configs", "../configs"}
    
    local home = os.getenv("USERPROFILE") or os.getenv("HOME")
    if home then
        table.insert(paths, home .. "/.autoinput")
    end

    for _, path in ipairs(paths) do
        local cmd = 'dir /b "' .. path .. '" 2>nul'
        local handle = io.popen(cmd)
        if handle then
            for line in handle:lines() do
                local name = line:match("^(.*)%.toml$")
                if name then
                    configs[name] = true
                end
            end
            handle:close()
        end
    end

    local results = {}
    for name, _ in pairs(configs) do
        table.insert(results, name)
    end
    return results
end

local log_levels = {"d", "debug", "i", "info", "w", "warn", "warning", "e", "error", "f", "fatal"}
local action_types = {"click", "c", "hold", "h"}
local mouse_buttons = {"left", "l", "right", "r", "middle", "m", "back", "forward"}
local notification_modes = {"off", "console", "desktop", "both"}
local modifiers = {"ctrl+", "shift+", "alt+", "meta+"}

local function with_modifiers(base_completions)
    local results = {}
    for _, base in ipairs(base_completions) do
        table.insert(results, base)
        for _, mod in ipairs(modifiers) do
            table.insert(results, mod..base)
        end
    end
    return results
end

local common_keys = {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12",
    "esc", "escape", "space", "tab", "enter", "return", "backspace", "ins", "insert",
    "del", "delete", "home", "end", "pageup", "pgup", "pagedown", "pgdn",
    "up", "down", "left", "right", "capslock", "numlock", "scrolllock", "printscreen", "prtsc", "pause"
}
local keys_with_mods = with_modifiers(common_keys)

-- Commands
local run_parser = clink.arg.new_parser()
run_parser:set_flags(
    "-c" .. clink.arg.new_parser({configs_matcher}),
    "--config" .. clink.arg.new_parser({configs_matcher}),
    "-t" .. clink.arg.new_parser(action_types),
    "--type" .. clink.arg.new_parser(action_types),
    "-b" .. clink.arg.new_parser(mouse_buttons),
    "--button" .. clink.arg.new_parser(mouse_buttons),
    "--btn" .. clink.arg.new_parser(mouse_buttons),
    "-k" .. clink.arg.new_parser(keys_with_mods),
    "--key" .. clink.arg.new_parser(keys_with_mods),
    "-n" .. clink.arg.new_parser(),
    "--name" .. clink.arg.new_parser(),
    "--control" .. clink.arg.new_parser(),
    "--control-action" .. clink.arg.new_parser(),
    "--control-input" .. clink.arg.new_parser(),
    "-s" .. clink.arg.new_parser(keys_with_mods),
    "--start" .. clink.arg.new_parser(keys_with_mods),
    "-e" .. clink.arg.new_parser(keys_with_mods),
    "--end" .. clink.arg.new_parser(keys_with_mods),
    "-a" .. clink.arg.new_parser(),
    "--app" .. clink.arg.new_parser(),
    "--application" .. clink.arg.new_parser(),
    "-B" .. clink.arg.new_parser(),
    "--blacklist" .. clink.arg.new_parser(),
    "-w" .. clink.arg.new_parser(),
    "--wait" .. clink.arg.new_parser(),
    "--press-wait" .. clink.arg.new_parser(),
    "--release-wait" .. clink.arg.new_parser(),
    "--status-notification" .. clink.arg.new_parser(notification_modes),
    "-S" .. clink.arg.new_parser({configs_matcher}),
    "--save-config" .. clink.arg.new_parser({configs_matcher})
)

local record_parser = clink.arg.new_parser()
record_parser:set_arguments({
    clink.arg.new_parser() -- name
})
record_parser:set_flags(
    "--start" .. clink.arg.new_parser(keys_with_mods),
    "--end" .. clink.arg.new_parser(keys_with_mods),
    "--play-start" .. clink.arg.new_parser(keys_with_mods),
    "--mouse-moves",
    "--mouse-sample" .. clink.arg.new_parser(),
    "--force"
)

local config_parser = clink.arg.new_parser()
config_parser:set_arguments({
    {
        "list",
        "validate" .. clink.arg.new_parser({configs_matcher}),
        "duplicate" .. clink.arg.new_parser({configs_matcher}, clink.arg.new_parser()),
        "copy" .. clink.arg.new_parser({configs_matcher}, clink.arg.new_parser()),
        "path" .. clink.arg.new_parser({configs_matcher})
    }
})
config_parser:setlags("--force")

local apps_parser = clink.arg.new_parser()
apps_parser:set_arguments({
    {"list"}
})

local serve_parser = clink.arg.new_parser()
serve_parser:set_flags(
    "--stdio"
)

local help_config_parser = clink.arg.new_parser()
help_config_parser:set_arguments({
    {"list", "validate", "duplicate", "copy", "path"}
})

local help_parser = clink.arg.new_parser()
help_parser:set_arguments({
    {
        "run",
        "record",
        "config" .. help_config_parser,
        "apps",
        "serve"
    }
})

local autoinput_parser = clink.arg.new_parser()
autoinput_parser:set_flags(
    "-h",
    "--help",
    "--examples",
    "-l" .. clink.arg.new_parser(log_levels),
    "--log" .. clink.arg.new_parser(log_levels),
    "--json"
)

autoinput_parser:set_arguments({
    {
        "run" .. run_parser,
        "record" .. record_parser,
        "config" .. config_parser,
        "apps" .. apps_parser,
        "serve" .. serve_parser,
        "help" .. help_parser
    }
})

clink.arg.register_parser("autoinput", autoinput_parser)
clink.arg.register_parser("autoinput.exe", autoinput_parser)
