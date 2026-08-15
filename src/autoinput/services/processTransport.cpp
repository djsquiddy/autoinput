/**
* @file processTransport.cpp
* @author djsquiddy
* @date August 2026
*/
#include "autoinput/services/processTransport.h"
#include "autoinput/support/logger.h"
#include <format>

namespace autoinput::services
{
#ifndef _WIN32
    struct StdioProcessTransport::Impl
    {
        std::string lastError;
    };

    std::string StdioProcessTransport::quoteWindowsArgument(const std::string& arg)
    {
        return arg;
    }

    StdioProcessTransport::StdioProcessTransport(std::filesystem::path executablePath)
        : m_executablePath(std::move(executablePath))
        , m_impl(std::make_unique<Impl>())
    {
    }

    StdioProcessTransport::StdioProcessTransport(std::filesystem::path executablePath, std::vector<std::string> arguments)
        : m_executablePath(std::move(executablePath))
        , m_arguments(std::move(arguments))
        , m_impl(std::make_unique<Impl>())
    {
    }

    StdioProcessTransport::~StdioProcessTransport()
    {
        stop();
    }

    bool StdioProcessTransport::start()
    {
        if (m_running)
        {
            return true;
        }

        m_impl->lastError = "StdioProcessTransport is not implemented for this platform.";
        return false;
    }

    void StdioProcessTransport::stop()
    {
        m_running = false;
    }

    bool StdioProcessTransport::running() const
    {
        return m_running;
    }

    std::string StdioProcessTransport::lastError() const
    {
        return m_impl ? m_impl->lastError : "";
    }

    bool StdioProcessTransport::writeLine(std::string_view line)
    {
        return false;
    }

    std::optional<std::string> StdioProcessTransport::readLine()
    {
        return readLine(std::chrono::milliseconds(0));
    }

    std::optional<std::string> StdioProcessTransport::readLine(std::chrono::milliseconds timeout)
    {
        return std::nullopt;
    }
#endif
}
