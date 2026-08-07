/**
 * @file config.cpp
 * @author djsquiddy
 * @date July 2026
 */
#define TOML_IMPLEMENTATION
#include "autoinput/config.h"

#include "errorCode.h"
#include "autoinput/utils.h"
#include "autoinput/logger.h"
#include "autoinput/platform.h"
#include "autoinput/environment.h"


namespace autoinput
{
    namespace
    {
        namespace fs = std::filesystem;

        void populateCommandData(CommandData& commandData, const toml::node& node)
        {
            const auto* table = node.as_table();
            if (!table)
            {
                return;
            }

            tryGetTableValue(*table, "name", commandData.name);
            tryGetTableValue(*table, "exclusiveGroup", commandData.exclusiveGroup);
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

        void populateRecordedEventData(RecordedEvent& event, const toml::node& node)
        {
            const auto* table = node.as_table();
            if (!table) return;

            std::string typeStr;
            if (tryGetTableValue(*table, "type", typeStr))
            {
                event.type = recordedEventTypeFromString(typeStr);
            }

            tryGetTableValue(*table, "delay", event.delay);
            
            std::string key;
            if (tryGetTableValue(*table, "key", key)) event.key = key;

            std::string button;
            if (tryGetTableValue(*table, "button", button)) event.button = button;

            int32_t x;
            if (tryGetTableValue(*table, "x", x)) event.x = x;

            int32_t y;
            if (tryGetTableValue(*table, "y", y)) event.y = y;
        }

        void populateSequenceData(RecordedSequence& sequence, const toml::node& node)
        {
            const auto* table = node.as_table();
            if (!table) return;

            tryGetTableValue(*table, "name", sequence.name);
            tryGetTableValue(*table, "start", sequence.start);
            tryGetTableValue(*table, "repeat", sequence.repeat);

            if (const auto eventsCfg = (*table)["events"])
            {
                if (const toml::array* eventsArr = eventsCfg.as_array())
                {
                    for (const toml::node& eventNode : *eventsArr)
                    {
                        RecordedEvent event;
                        populateRecordedEventData(event, eventNode);
                        sequence.events.emplace_back(std::move(event));
                    }
                }
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
                tryGetTableValue(cmdNode, "statusNotificationMode", configData.statusNotificationMode);
                tryGetTableValue(cmdNode, "logLevel", configData.logLevel);

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
            tryGetTableValue(table, "statusNotificationMode", configData.statusNotificationMode);
            tryGetTableValue(table, "logLevel", configData.logLevel);
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

            const auto sequenceNode = table["sequence"];
            if (sequenceNode.is_table())
            {
                RecordedSequence seq;
                populateSequenceData(seq, *sequenceNode.as_table());
                configData.sequences.emplace_back(std::move(seq));
            }
            else if (sequenceNode.is_array())
            {
                for (toml::node& item : *sequenceNode.as_array())
                {
                    RecordedSequence seq;
                    populateSequenceData(seq, item);
                    configData.sequences.emplace_back(std::move(seq));
                }
            }
        }
    }

    fs::path getConfigsPath()
    {
        return getConfigsPath(SystemEnvironment::instance());
    }

    fs::path getConfigsPath(const IEnvironment& environment)
    {
        static fs::path configsPath;
        if (configsPath.empty())
        {
            configsPath = environment.executablePath() / "configs";
        }
        return configsPath;
    }
    
    fs::path getUserConfigsPath()
    {
        return getUserConfigsPath(SystemEnvironment::instance());
    }

    fs::path getUserConfigsPath(const IEnvironment& environment)
    {
        const fs::path home = environment.userHomePath();
        if (home.empty())
        {
            return {};
        }
        return home / ".autoinput";
    }

    fs::path getConfigFilePath(const std::string& filePath)
    {
        fs::path configFilePath{ filePath };
        if (configFilePath.extension() != ".toml")
        {
            configFilePath = filePath + ".toml";
        }

        if (fs::path userPath = getUserConfigsPath() / configFilePath; fs::exists(userPath))
        {
            return userPath;
        }

        const fs::path globalPath = getConfigsPath() / configFilePath;
        if (fs::exists(globalPath))
        {
            return globalPath;
        }

        Logger::error("Configuration could not be found: {}", filePath);
        return globalPath;
    }

    std::optional<ConfigData> loadConfigData(const fs::path& configPath)
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

    bool doesConfigDataExists(const fs::path& configPath)
    {
        return fs::exists(configPath);
    }

    bool saveConfigData(const ConfigData& configData, const fs::path& configPath, const std::optional<DefaultSettings>& defaults)
    {
        toml::table table;

        auto createCommandTable = [&](const CommandData& cmd) {
            toml::table t;
            if (!cmd.name.empty())
            {
                t.insert("name", cmd.name);
            }
            if (!cmd.exclusiveGroup.empty())
            {
                t.insert("exclusiveGroup", cmd.exclusiveGroup);
            }
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
                waitTime.is_inline(true);
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

        auto createSequenceTable = [&](const RecordedSequence& seq) {
            toml::table t;
            if (!seq.name.empty())
            {
                t.insert("name", seq.name);
            }
            t.insert("start", seq.start);
            t.insert("repeat", seq.repeat);

            toml::array eventsArr;
            for (const auto& event : seq.events)
            {
                toml::table et;
                et.is_inline(true);
                et.insert("type", recordedEventTypeToString(event.type));
                et.insert("delay", event.delay);
                if (event.key.has_value()) et.insert("key", *event.key);
                if (event.button.has_value()) et.insert("button", *event.button);
                if (event.x.has_value()) et.insert("x", *event.x);
                if (event.y.has_value()) et.insert("y", *event.y);
                eventsArr.push_back(std::move(et));
            }
            t.insert("events", std::move(eventsArr));
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

        if (!configData.sequences.empty())
        {
            if (configData.sequences.size() == 1)
            {
                table.insert("sequence", createSequenceTable(configData.sequences[0]));
            }
            else
            {
                toml::array sequenceArray;
                for (const auto& seq : configData.sequences)
                {
                    sequenceArray.push_back(createSequenceTable(seq));
                }
                table.insert("sequence", std::move(sequenceArray));
            }
        }

        if (!configPath.parent_path().empty() && !fs::exists(configPath.parent_path()))
        {
            fs::create_directories(configPath.parent_path());
        }

        if (!configData.statusNotificationMode.empty())
        {
            table.insert_or_assign("statusNotificationMode", configData.statusNotificationMode);
        }

        if (!configData.logLevel.empty())
        {
            table.insert_or_assign("logLevel", configData.logLevel);
        }

        std::ofstream file(configPath);
        if (!file.is_open())
        {
            return false;
        }

        file << table;
        return true;
    }

    bool duplicateConfig(const std::string& sourceNameOrPath, const std::string& destinationNameOrPath, const bool overwrite)
    {
        const fs::path sourcePath = getConfigFilePath(sourceNameOrPath);
        if (!fs::exists(sourcePath))
        {
            Logger::error("Source configuration not found: {}", sourceNameOrPath);
            return false;
        }

        fs::path destPath = getUserConfigsPath() / fs::path(destinationNameOrPath).filename();
        if (destPath.extension() != ".toml")
        {
            destPath.replace_extension(".toml");
        }

        if (destPath.filename() == "settings.toml")
        {
            Logger::error("Cannot duplicate to settings.toml as it is protected.");
            return false;
        }

        Logger::debug("Duplicating from {} to {}", sourcePath.string(), destPath.string());

        // Check if source and destination are the same
        std::error_code ec;
        if (fs::exists(destPath) && fs::equivalent(sourcePath, destPath, ec))
        {
            Logger::error("Source and destination resolve to the same path: {}", sourcePath.string());
            return false;
        }

        if (fs::exists(destPath) && !overwrite)
        {
            Logger::error("Destination configuration already exists: {}", destPath.string());
            return false;
        }

        if (!destPath.parent_path().empty() && !fs::exists(destPath.parent_path()))
        {
            if (!fs::create_directories(destPath.parent_path(), ec))
            {
                Logger::error("Failed to create directory {}: {}", destPath.parent_path().string(), ec.message());
                return false;
            }
        }

        try
        {
            fs::copy_file(sourcePath, destPath, overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none);
            Logger::print("Configuration duplicated from {} to {}\n", sourcePath.string(), destPath.string());
            return true;
        }
        catch (const fs::filesystem_error& e)
        {
            Logger::error("Failed to duplicate configuration: {}", e.what());
            return false;
        }
    }

    std::string_view configTypeToString(const ConfigType configType)
    {
        switch (configType)
        {
        case ConfigType::Global:
            return "Global";
        case ConfigType::User:
            return "User";
        case ConfigType::Unknown:
        default:
            return "";
        }
    }

    fs::path configTypeToPath(const ConfigType configType)
    {
        switch (configType)
        {
        case ConfigType::Global:
            return getConfigsPath();
        case ConfigType::User:
            return getUserConfigsPath();
        case ConfigType::Unknown:
        default:
            return {};
        }
    }

    fs::path configTypeToPath(const ConfigType configType, const IEnvironment& environment)
    {
        switch (configType)
        {
        case ConfigType::Global:
            return getConfigsPath(environment);
        case ConfigType::User:
            return getUserConfigsPath(environment);
        case ConfigType::Unknown:
        default:
            return {};
        }
    }

    std::string ConfigInfo::fileName() const
    {
        return filepath.filename().string();
    }

    std::string ConfigInfo::fileStem() const
    {
        return filepath.stem().string();
    }

    ConfigService::ConfigService(const IEnvironment& environment)
        : m_environment{ environment }
    {
    }

    std::vector<ConfigInfo> ConfigService::listAvailableConfigs(ConfigType configType) const
    {
        const auto path = configTypeToPath(configType, m_environment);
        const auto label = configTypeToString(configType);
        Logger::debug("{} configurations", label);

        std::vector<ConfigInfo> configs{};

        if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
        {
            return configs;
        }

        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".toml")
            {
                continue;
            }

            const std::string name = entry.path().stem().string();
            if (name == "settings")
            {
                continue;
            }

            Logger::debug("Found config: {}\n", name);
            configs.emplace_back(ConfigInfo{
                    .type = configType,
                    .filepath = fs::path(entry.path())
            });
        }

        return configs;
    }

    ValidationResult ConfigService::validateConfig(const std::string& source) const
    {
        const auto configPath = getConfigFilePath(source);

        if (!doesConfigDataExists(configPath))
        {
            return ValidationResult{
                .isValid = false,
                .configPath = configPath.string(),
                .errors = {{ .message = "Configuration file not found" } }
            };
        }

        const auto configData = loadConfigData(configPath);
        if (!configData.has_value())
        {
            return ValidationResult{
                .isValid = false,
                .configPath = configPath.string(),
                .errors = {{ .message = "Failed to load configuration file" } }
            };
        }

        auto errors = validateConfigData(*configData);
        if (errors.empty())
        {
            Logger::info("Configuration is valid: {}", source);
            return ValidationResult{
                .isValid = true,
                .configPath = configPath.string()
            };
        }

        errors.emplace_back(ValidationError{.message = "Configuration validation failed"});
        return ValidationResult{
            .isValid = false,
            .configPath = configPath.string(),
            .errors = std::move(errors)
        };
    }

    ProgramArguments* ConfigService::loadConfigAsArguments(std::string_view source)
    {
        return nullptr;
    }

    std::vector<ConfigInfo> ConfigService::listAvailableConfigs() const
    {
        std::vector<ConfigInfo> configs = listAvailableConfigs(ConfigType::Global);
        configs.append_range(listAvailableConfigs(ConfigType::User));
        return configs;
    }

}
