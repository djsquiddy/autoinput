/**
 * @file configService.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/services/configService.h"

#include "autoinput/arguments.h"
#include "autoinput/configMetadata.h"
#include "autoinput/logger.h"

namespace autoinput::services
{
    namespace
    {
        namespace fs = std::filesystem;
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

    std::unique_ptr<ProgramArguments> ConfigService::loadConfigAsArguments(const std::string_view source) const
    {
        auto args = std::make_unique<ProgramArguments>();
        if (applyConfigToArguments(source, *args))
        {
            return args;
        }
        return nullptr;
    }

    bool ConfigService::applyConfigToArguments(const std::string_view source, ProgramArguments& arguments) const
    {
        if (source.empty())
        {
            return true;
        }

        m_currentConfig = std::string(source);
        const auto configPath = getConfigFilePath(std::string(source), m_environment);

        if (!doesConfigDataExists(configPath))
        {
            return false;
        }

        const auto foundConfigData = loadConfigData(configPath);
        if (!foundConfigData.has_value())
        {
            return false;
        }

        const ConfigData& configData = *foundConfigData;

        for (const CommandData& cmd : configData.commands)
        {
            const auto state = actionStateFromArguments(cmd.action);
            const auto action = state != ActionState::INVALID ? state : ActionState::CLICK;

            for (const std::string& button : cmd.buttons)
            {
                const auto mouse = Mouse::fromString(button);
                if (mouse.button == MouseButton::None)
                {
                    Logger::error("Invalid parameter {} for button type. Choices: {}\n", button, ConfigMetadata::validMouseButtonChoices());
                    return false;
                }

                arguments.buttons.push_back(mouse);
                arguments.targetActions.push_back(action);
                arguments.commandNames.push_back(cmd.name);
                arguments.exclusiveGroups.push_back(cmd.exclusiveGroup);
            }

#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
            for (const std::string& key : cmd.keys)
            {
                arguments.keys.push_back(Key::fromString(key));
                arguments.targetActions.push_back(action);
                arguments.commandNames.push_back(cmd.name);
                arguments.exclusiveGroups.push_back(cmd.exclusiveGroup);
            }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED

            for (const std::string& startKey : cmd.startKeys)
            {
                arguments.startKeys.push_back(startKey);
            }

            if (!cmd.pressWait.empty())
            {
                if (!arguments.delayData.parseWaitTimeDelay(cmd.pressWait, true))
                {
                    return false;
                }
            }

            if (!cmd.releaseWait.empty())
            {
                if (!arguments.delayData.parseWaitTimeDelay(cmd.releaseWait, false))
                {
                    return false;
                }
            }
        }

        if (!configData.endKey.empty())
        {
            arguments.endKey = configData.endKey;
        }

        if (!configData.application.empty())
        {
            if (m_applicationName.empty())
            {
                arguments.applicationName = m_applicationName = configData.application;
            }
            else
            {
                arguments.applicationName = configData.application;
            }
        }

        if (!configData.blacklist.empty())
        {
            if (!configData.appendBlacklist)
            {
                arguments.blacklist.clear();
            }

            arguments.blacklist.insert(
                arguments.blacklist.end(),
                configData.blacklist.begin(),
                configData.blacklist.end()
            );
        }

        if (!configData.statusNotificationMode.empty())
        {
            arguments.statusNotificationMode = statusNotificationModeFromString(configData.statusNotificationMode);
        }

        if (!configData.sequences.empty())
        {
            arguments.sequences.insert(
                arguments.sequences.end(),
                configData.sequences.begin(),
                configData.sequences.end()
            );
        }

        return arguments.postParseArguments();
    }

    std::vector<ConfigInfo> ConfigService::listAvailableConfigs() const
    {
        std::vector<ConfigInfo> configs = listAvailableConfigs(ConfigType::Global);
        configs.append_range(listAvailableConfigs(ConfigType::User));
        return configs;
    }

}