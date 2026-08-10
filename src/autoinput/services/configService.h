/**
 * @file configService.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_SERVICES_CONFIGSERVICE_H
#define INCLUDE_AUTOINPUT_SERVICES_CONFIGSERVICE_H
#pragma once

#include "autoinput/config/config.h"

namespace autoinput
{
    class ProgramArguments;
}

namespace autoinput::services
{
    struct ConfigInfo
    {
        ConfigType type{ ConfigType::Unknown };
        std::filesystem::path filepath;

        [[nodiscard]] bool isValid() const { return type != ConfigType::Unknown; }
        [[nodiscard]] std::string fileName() const;
        [[nodiscard]] std::string fileStem() const;
    };

    class ConfigService
    {
    public:
        explicit ConfigService(const IEnvironment& environment);

        [[nodiscard]] std::optional<std::string> getApplicationName() const { if (m_applicationName.empty()) return std::nullopt; return m_applicationName; }
        void setApplicationName(std::string applicationName) { m_applicationName = std::move(applicationName); }
        [[nodiscard]] std::vector<ConfigInfo> listAvailableConfigs() const;
        [[nodiscard]] std::vector<ConfigInfo> listAvailableConfigs(ConfigType configType) const;
        [[nodiscard]] ValidationResult validateConfig(const std::string& source) const;
        [[nodiscard]] std::unique_ptr<ProgramArguments> loadConfigAsArguments(std::string_view source) const;
        [[nodiscard]] bool applyConfigToArguments(std::string_view source, ProgramArguments& arguments) const;
        [[nodiscard]] const std::string& getCurrentConfig() const { return m_currentConfig; }

    private:
        const IEnvironment& m_environment;
        mutable std::string m_currentConfig{};
        mutable std::string m_applicationName{};
    };
}


#endif // INCLUDE_AUTOINPUT_SERVICES_CONFIGSERVICE_H
