/**
 * @file config.cpp
 * @author djsquiddy
 * @date July 2026
 */
#define TOML_IMPLEMENTATION
#include "autoinput/config.h"

#include "utils.h"
#include "autoinput/logger.h"
#include "autoinput/platform.h"

namespace autoinput
{
    namespace
    {
        void populateCommandData(CommandData& commandData, const toml::node& node)
        {
            const auto* table = node.as_table();
            if (!table)
            {
                return;
            }

            tryGetTableValue(*table, "action", commandData.action);
            if (const auto buttonCfg = (*table)["button"])
            {
                if (buttonCfg.is_string())
                {
                    std::string btn;
                    tryGetTableValue(*table, "button", btn);
                    commandData.buttons.emplace_back(btn);
                }
                else if (const toml::array* btns = buttonCfg.as_array())
                {
                    for (const toml::node& btn : *btns)
                    {
                        commandData.buttons.emplace_back(btn.as_string()->value_or(""));
                    }
                }
            }
            if (const auto startCfg = (*table)["start"])
            {
                if (startCfg.is_string())
                {
                    std::string btn;
                    tryGetTableValue(*table, "start", btn);
                    commandData.startKeys.emplace_back(btn);
                }
                else if (const toml::array* btns = startCfg.as_array())
                {
                    for (const toml::node& btn : *btns)
                    {
                        commandData.startKeys.emplace_back(btn.as_string()->value_or(""));
                    }
                }
            }
            if (const auto keyCfg = (*table)["key"])
            {
                if (keyCfg.is_string())
                {
                    std::string key;
                    tryGetTableValue(*table, "key", key);
                    commandData.keys.emplace_back(key);
                }
                else if (const toml::array* ks = keyCfg.as_array())
                {
                    for (const toml::node& k : *ks)
                    {
                        commandData.keys.emplace_back(k.as_string()->value_or(""));
                    }
                }
            }
            if (const auto waitTime = (*table)["time"].as_table())
            {
                tryGetTableValue(*waitTime, "press", commandData.pressWait);
                tryGetTableValue(*waitTime, "release", commandData.releaseWait);
            }
        }

        void populateConfigData(ConfigData& configData, toml::table& table)
        {
            const auto cmdNode = table["command"];
            if (cmdNode.is_table())
            {
                CommandData cmd;
                populateCommandData(cmd, *cmdNode.as_table());
                configData.commands.emplace_back(std::move(cmd));

                // For backward compatibility, check for global fields inside the single command table
                tryGetTableValue(cmdNode, "end", configData.endKey);
                tryGetTableValue(cmdNode, "application", configData.application);
                tryGetTableValue(cmdNode, "appendBlacklist", configData.appendBlacklist);

                if (const auto blacklistCfg = cmdNode["blacklist"])
                {
                    if (blacklistCfg.is_string())
                    {
                        std::string app;
                        tryGetTableValue(cmdNode, "blacklist", app);
                        configData.blacklist.emplace_back(app);
                    }
                    else if (toml::array* apps = blacklistCfg.as_array())
                    {
                        for (toml::node& app : *apps)
                        {
                            configData.blacklist.emplace_back(app.as_string()->value_or(""));
                        }
                    }
                }
            }
            else if (cmdNode.is_array())
            {
                for (toml::node& item : *cmdNode.as_array())
                {
                    CommandData cmd;
                    populateCommandData(cmd, item);
                    configData.commands.emplace_back(std::move(cmd));
                }
            }

            // Also allow global fields at the top level
            tryGetTableValue(table, "end", configData.endKey);
            tryGetTableValue(table, "application", configData.application);
            tryGetTableValue(table, "appendBlacklist", configData.appendBlacklist);
            if (const auto blacklistCfg = table["blacklist"])
            {
                if (blacklistCfg.is_string())
                {
                    std::string app;
                    tryGetTableValue(table, "blacklist", app);
                    configData.blacklist.emplace_back(app);
                }
                else if (toml::array* apps = blacklistCfg.as_array())
                {
                    for (toml::node& app : *apps)
                    {
                        configData.blacklist.emplace_back(app.as_string()->value_or(""));
                    }
                }
            }
        }
    }

    const std::filesystem::path& getConfigsPath()
    {
        static const std::filesystem::path configsPath = platform::getExecutablePath() / "configs";
        return configsPath;
    }
    
    std::filesystem::path getUserConfigsPath()
    {
        const std::filesystem::path home = platform::getUserHomePath();
        if (home.empty())
        {
            return {};
        }
        return home / ".autoinput";
    }

    std::filesystem::path getConfigFilePath(const std::string& filePath)
    {
        std::filesystem::path configFilePath{ filePath };
        if (configFilePath.extension() != ".toml")
        {
            configFilePath = filePath + ".toml";
        }

        const std::filesystem::path userPath = getUserConfigsPath() / configFilePath;
        if (std::filesystem::exists(userPath))
        {
            return userPath;
        }

        const std::filesystem::path globalPath = getConfigsPath() / configFilePath;
        if (std::filesystem::exists(globalPath))
        {
            return globalPath;
        }

        Logger::error("Configuration could not be found: {}", filePath);
        return globalPath;
    }

    std::optional<ConfigData> loadConfigData(const std::filesystem::path& configPath)
    {
        toml::parse_result result = toml::parse_file(configPath.string());
        if (!result)
        {
            Logger::errorStream() << "Parsing failed:\n" << result.error();
            return std::nullopt;
        }

        toml::table table = std::move(result).table();
        ConfigData configData;
        populateConfigData(configData, table);
        return configData;
    }

    bool doesConfigDataExists(const std::filesystem::path& configPath)
    {
        return std::filesystem::exists(configPath);
    }

    bool saveConfigData(const ConfigData& configData, const std::filesystem::path& configPath, const std::optional<DefaultSettings>& defaults)
    {
        toml::table table;

        auto createCommandTable = [&](const CommandData& cmd) {
            toml::table t;
            if (!cmd.action.empty())
            {
                if (!defaults.has_value() || cmd.action != defaults->action)
                {
                    t.insert("action", cmd.action);
                }
            }

            auto insertStringOrArrayToTable = [&](toml::table& target, const std::string_view key, const std::vector<std::string>& values, const std::optional<std::string>& defaultValue = std::nullopt) {
                if (defaultValue.has_value() && values.size() == 1 && values[0] == *defaultValue)
                {
                    return;
                }
                
                if (values.size() == 1)
                {
                    target.insert(key, values[0]);
                }
                else if (!values.empty())
                {
                    toml::array arr;
                    for (const auto& v : values)
                    {
                        arr.push_back(v);
                    }
                    target.insert(key, std::move(arr));
                }
            };

            insertStringOrArrayToTable(t, "button", cmd.buttons, defaults.has_value() ? std::make_optional(defaults->button) : std::nullopt);
            insertStringOrArrayToTable(t, "key", cmd.keys);
            insertStringOrArrayToTable(t, "start", cmd.startKeys, defaults.has_value() ? std::make_optional(defaults->start) : std::nullopt);

            if (!cmd.pressWait.empty() || !cmd.releaseWait.empty())
            {
                toml::table waitTime;
                if (!cmd.pressWait.empty())
                {
                    if (!defaults.has_value() || cmd.pressWait != defaults->press)
                    {
                        waitTime.insert("press", cmd.pressWait);
                    }
                }
                if (!cmd.releaseWait.empty())
                {
                    if (!defaults.has_value() || cmd.releaseWait != defaults->release)
                    {
                        waitTime.insert("release", cmd.releaseWait);
                    }
                }
                if (!waitTime.empty())
                {
                    t.insert("time", std::move(waitTime));
                }
            }
            return t;
        };

        auto filterBlacklist = [&](const std::vector<std::string>& blacklist) {
            if (!defaults.has_value())
            {
                return blacklist;
            }

            std::vector<std::string> filtered;
            for (const auto& item : blacklist)
            {
                if (!contains(defaults->blacklist, item))
                {
                    filtered.push_back(item);
                }
            }
            return filtered;
        };

        const auto filteredBlacklist = filterBlacklist(configData.blacklist);

        if (configData.commands.size() == 1)
        {
            toml::table command = createCommandTable(configData.commands[0]);
            // Put global fields inside the command table for backward compatibility when there's only one command
            if (!configData.endKey.empty())
            {
                if (!defaults.has_value() || configData.endKey != defaults->end)
                {
                    command.insert("end", configData.endKey);
                }
            }
            if (!configData.application.empty())
            {
                command.insert("application", configData.application);
            }
            
            if (!configData.appendBlacklist)
            {
                command.insert("appendBlacklist", false);
            }
            
            auto insertStringOrArrayToTable = [&](toml::table& target, const std::string_view key, const std::vector<std::string>& values) {
                if (values.size() == 1)
                {
                    target.insert(key, values[0]);
                }
                else if (!values.empty())
                {
                    toml::array arr;
                    for (const auto& v : values)
                    {
                        arr.push_back(v);
                    }
                    target.insert(key, std::move(arr));
                }
            };
            insertStringOrArrayToTable(command, "blacklist", filteredBlacklist);

            table.insert("command", std::move(command));
        }
        else if (!configData.commands.empty())
        {
            toml::array commandArray;
            for (const auto& cmd : configData.commands)
            {
                commandArray.push_back(createCommandTable(cmd));
            }
            table.insert("command", std::move(commandArray));

            // Put global fields at top level when there are multiple commands
            if (!configData.endKey.empty())
            {
                if (!defaults.has_value() || configData.endKey != defaults->end)
                {
                    table.insert("end", configData.endKey);
                }
            }
            if (!configData.application.empty())
            {
                table.insert("application", configData.application);
            }
            
            if (!configData.appendBlacklist)
            {
                table.insert("appendBlacklist", false);
            }

            auto insertStringOrArrayToTable = [&](toml::table& target, const std::string_view key, const std::vector<std::string>& values) {
                if (values.size() == 1)
                {
                    target.insert(key, values[0]);
                }
                else if (!values.empty())
                {
                    toml::array arr;
                    for (const auto& v : values)
                    {
                        arr.push_back(v);
                    }
                    target.insert(key, std::move(arr));
                }
            };
            insertStringOrArrayToTable(table, "blacklist", filteredBlacklist);
        }

        if (!configPath.parent_path().empty() && !std::filesystem::exists(configPath.parent_path()))
        {
            std::filesystem::create_directories(configPath.parent_path());
        }

        std::ofstream file(configPath);
        if (!file.is_open())
        {
            return false;
        }

        file << table;
        return true;
    }
}
