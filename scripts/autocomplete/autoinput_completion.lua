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

local log_levels = {"debug", "info", "warn", "warning", "error", "fatal", "d", "i", "w", "e", "f"}
local action_types = {"click", "hold", "c", "h"}
local mouse_buttons = {"left", "right", "middle", "back", "forward", "l", "r", "m"}
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

local mouse_buttons_with_mods = with_modifiers(mouse_buttons)
local common_keys = {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12",
    "enter", "space", "tab", "backspace", "escape", "delete", "insert", "home", "end", "pageup", "pagedown",
    "up", "down", "left", "right"
}
local keys_with_mods = with_modifiers(common_keys)

local all_pos = {}
for _, v in ipairs(action_types) do table.insert(all_pos, v) end
for _, v in ipairs(mouse_buttons_with_mods) do table.insert(all_pos, v) end
for _, v in ipairs(keys_with_mods) do table.insert(all_pos, v) end

local autoinput_parser = clink.arg.new_parser()
autoinput_parser:set_flags(
    "-h", "--help",
    "-l" .. clink.arg.new_parser(log_levels),
    "--log" .. clink.arg.new_parser(log_levels),
    "-c" .. clink.arg.new_parser({configs_matcher}),
    "--config" .. clink.arg.new_parser({configs_matcher}),
    "-t" .. clink.arg.new_parser(action_types),
    "--type" .. clink.arg.new_parser(action_types),
    "-b" .. clink.arg.new_parser(mouse_buttons_with_mods),
    "--btn" .. clink.arg.new_parser(mouse_buttons_with_mods),
    "--button" .. clink.arg.new_parser(mouse_buttons_with_mods),
    "-k" .. clink.arg.new_parser(keys_with_mods),
    "--key" .. clink.arg.new_parser(keys_with_mods),
    "-s" .. clink.arg.new_parser(keys_with_mods),
    "--start" .. clink.arg.new_parser(keys_with_mods),
    "--start-key" .. clink.arg.new_parser(keys_with_mods),
    "-e" .. clink.arg.new_parser(keys_with_mods),
    "--end" .. clink.arg.new_parser(keys_with_mods),
    "--end-key" .. clink.arg.new_parser(keys_with_mods),
    "-a" .. clink.arg.new_parser(),
    "--app" .. clink.arg.new_parser(),
    "--application" .. clink.arg.new_parser(),
    "-B" .. clink.arg.new_parser(),
    "--blacklist" .. clink.arg.new_parser(),
    "-L", "--list-apps",
    "-C", "--list-configs",
    "-S" .. clink.arg.new_parser({configs_matcher}),
    "--save-config" .. clink.arg.new_parser({configs_matcher}),
    "--validate-config" .. clink.arg.new_parser({configs_matcher}),
    "--duplicate-config" .. clink.arg.new_parser({configs_matcher}, {configs_matcher}),
    "--copy-config" .. clink.arg.new_parser({configs_matcher}, {configs_matcher}),
    "--force",
    "--json",
    "-w" .. clink.arg.new_parser(),
    "--wait" .. clink.arg.new_parser(),
    "--press-wait" .. clink.arg.new_parser(),
    "--release-wait" .. clink.arg.new_parser()
)

autoinput_parser:set_arguments({
    clink.arg.new_parser(all_pos):loop(10)
})

clink.arg.register_parser("autoinput", autoinput_parser)
clink.arg.register_parser("run.cmd", autoinput_parser)
