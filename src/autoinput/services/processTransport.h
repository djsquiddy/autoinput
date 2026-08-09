/**
* @file processTransport.h
* @author djsquiddy
* @date August 2026
*/
#ifndef INCLUDE_AUTOINPUT_SERVICE_PROCESS_TRANSPORT_H
#define INCLUDE_AUTOINPUT_SERVICE_PROCESS_TRANSPORT_H
#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <filesystem>
#include <memory>
#include <chrono>

namespace autoinput::services
{
    class IProcessTransport
    {
    public:
        virtual ~IProcessTransport() = default;
        virtual bool start() = 0;
        virtual void stop() = 0;
        [[nodiscard]] virtual bool running() const = 0;
        [[nodiscard]] virtual std::string lastError() const = 0;

        virtual bool writeLine(std::string_view line) = 0;
        [[nodiscard]] virtual std::optional<std::string> readLine() = 0;

        /**
         * @brief Read a line from the transport with an optional timeout.
         * @param timeout The maximum time to wait for a line. A timeout of 0 means blocking/no timeout.
         * @return The line read, or std::nullopt if the transport is not running, 
         *         a timeout occurs, or an error happens.
         */
        [[nodiscard]] virtual std::optional<std::string> readLine(std::chrono::milliseconds timeout)
        {
            // Default implementation just calls the blocking read
            return readLine();
        }
    };

    class StdioProcessTransport final : public IProcessTransport
    {
    public:
        explicit StdioProcessTransport(std::filesystem::path executablePath);
        StdioProcessTransport(std::filesystem::path executablePath, std::vector<std::string> arguments);
        ~StdioProcessTransport() override;

        bool start() override;
        void stop() override;
        [[nodiscard]] bool running() const override;
        [[nodiscard]] std::string lastError() const override;

        bool writeLine(std::string_view line) override;
        [[nodiscard]] std::optional<std::string> readLine() override;
        [[nodiscard]] std::optional<std::string> readLine(std::chrono::milliseconds timeout) override;
        
        // Testing helper
        static std::string quoteWindowsArgument(const std::string& arg);

    private:
        std::filesystem::path m_executablePath;
        std::vector<std::string> m_arguments;
        bool m_running{ false };
        
        // Implementation details for platform specific process handling would go here
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}

#endif // INCLUDE_AUTOINPUT_SERVICE_PROCESS_TRANSPORT_H
