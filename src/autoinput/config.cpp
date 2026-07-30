/**
 * @file config.cpp
 * @author djsquiddy
 * @date July 2026
 */
#define TOML_IMPLEMENTATION
#include "config.h"
#include "logger.h"
#include "platform.h"

namespace autoinput
{
    namespace
    {
        void populateConfigData(ConfigData& configData, toml::table& table)
        {
            const auto cmd = table["command"];
            tryGetTableValue(cmd, "action", configData.action);
            if (const auto buttonCfg = cmd["button"])
            {
                if (buttonCfg.is_string())
                {
                    std::string btn;
                    tryGetTableValue(cmd, "button", btn);
                    configData.buttons.emplace_back(btn);
                }
                else if (toml::array* btns = buttonCfg.as_array())
                {
                    for (toml::node& btn : *btns)
                    {
                        configData.buttons.emplace_back(btn.as_string()->value_or(""));
                    }
                }
            }
            if (const auto startCfg = cmd["start"])
            {
                if (startCfg.is_string())
                {
                    std::string btn;
                    tryGetTableValue(cmd, "start", btn);
                    configData.startKeys.emplace_back(btn);
                }
                else if (toml::array* btns = startCfg.as_array())
                {
                    for (toml::node& btn : *btns)
                    {
                        configData.startKeys.emplace_back(btn.as_string()->value_or(""));
                    }
                }
            }
            if (const auto keyCfg = cmd["key"])
            {
                if (keyCfg.is_string())
                {
                    std::string key;
                    tryGetTableValue(cmd, "key", key);
                    configData.keys.emplace_back(key);
                }
                else if (toml::array* ks = keyCfg.as_array())
                {
                    for (toml::node& k : *ks)
                    {
                        configData.keys.emplace_back(k.as_string()->value_or(""));
                    }
                }
            }
            tryGetTableValue(cmd, "end", configData.endKey);
            tryGetTableValue(cmd, "application", configData.application);
            if (const auto blacklistCfg = cmd["blacklist"])
            {
                if (blacklistCfg.is_string())
                {
                    std::string app;
                    tryGetTableValue(cmd, "blacklist", app);
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
            if (const auto waitTime = cmd["time"].as_table())
            {
                tryGetTableValue(*waitTime, "press", configData.pressWait);
                tryGetTableValue(*waitTime, "release", configData.releaseWait);
            }
        }
    }

    const std::filesystem::path& getConfigsPath()
    {
        static const std::filesystem::path configsPath = platform::getExecutablePath() / "configs";
        return configsPath;
    }

    std::filesystem::path getConfigFilePath(const std::string& filePath)
    {
        std::filesystem::path configFilePath{ filePath };
        if (configFilePath.extension() != ".toml")
        {
            configFilePath = filePath + ".toml";
        }
        return getConfigsPath() / configFilePath;
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
}
