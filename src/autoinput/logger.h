/**
 * @file config.cpp
 * @author djsquiddy
 * @date July 2026
 */
#ifndef INCLUDE_AUTOINPUT_LOGGER_H
#define INCLUDE_AUTOINPUT_LOGGER_H
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <iostream>
#include <sstream>
#include <fstream>
#include <source_location>
#include <format>
#include <atomic>
#include <mutex>
#include <type_traits>

namespace autoinput
{
    class Logger;

    enum class LogLevel : uint8_t
    {
        Debug = 0,
        Info = 1,
        Print = 2,
        Warning = 3,
        Error = 4,
        Fatal = 5,
        Unknown = 6
    };

    LogLevel logLevelFromString(std::string_view str);
    std::string getLogLevelPrefix(LogLevel level, bool isShorthand = false);

    // Overload the insertion operator
    inline std::ostream& operator<<(std::ostream& os, const LogLevel level) { return os << getLogLevelPrefix(level); }
    std::ostream& getConsoleStream(LogLevel level);

    // Helper class to enable: Logger::info << "msg";
    class LogStream
    {
    public:
        LogStream(Logger& logger, LogLevel level, std::source_location loc);

        // Capture the destructor to flush the log (optional, or flush on every <<)
        ~LogStream();

        // Template operator<< to accept any streamable type
        template <typename T>
        LogStream& operator<<(const T& value)
        {
            m_buffer << value;
            return *this;
        }

    private:
        Logger& m_logger;
        LogLevel m_level;
        std::source_location m_loc;
        std::ostringstream m_buffer;
    };

    // Helper for format strings that captures source location
    template <typename... Args>
    struct Fmt
    {
        std::format_string<Args...> value;
        std::source_location loc;

        // ReSharper disable once CppNonExplicitConvertingConstructor
        template <typename T>
        consteval Fmt(const T& s, const std::source_location l = std::source_location::current())
            : value(s)
            , loc(l)
        {
        }
    };

    class Logger
    {
    public:
        // Delete copy and move operations to enforce singleton
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        static Logger& instance();

        // Generic log method with format arguments
        static void log(LogLevel level, std::string_view msg, std::source_location loc = std::source_location::current());

        template <typename... Args>
        static void log(LogLevel level, Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        // Convenience wrappers for specific levels
        static void print(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void print(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        static void info(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void info(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        static void debug(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void debug(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        static void warn(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void warn(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        static void error(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void error(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        static void fatal(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void fatal(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        static LogStream debugStream(std::source_location loc = std::source_location::current());
        static LogStream printStream(std::source_location loc = std::source_location::current());
        static LogStream infoStream(std::source_location loc = std::source_location::current());
        static LogStream warnStream(std::source_location loc = std::source_location::current());
        static LogStream errorStream(std::source_location loc = std::source_location::current());
        static LogStream fatalStream(std::source_location loc = std::source_location::current());

        static bool isDebugModeEnabled();

        static void setLogLevel(LogLevel logLevel);
        static LogLevel getLogLevel();
        static void flush();
        static void setTesting(bool testing);
        static bool isTesting();
        static void setConsoleOutputEnabled(bool enabled);
        static bool isConsoleOutputEnabled();
        // Optional: Allow configuring the file path at runtime
        static void setFile(const std::string& filename);
        static const std::string& getFileName();

    private:
        friend class LogStream;
        // Private constructor
        Logger();

        ~Logger();

        void log_impl(LogLevel level, std::string_view msg, std::source_location loc = std::source_location::current());
        void flush_impl();

        void setLogLevel_impl(LogLevel logLevel);
        LogLevel getLogLevel_impl() const;

        void setTesting_impl(bool testing);
        bool isTesting_impl() const;

        void setConsoleOutputEnabled_impl(bool enabled);
        bool isConsoleOutputEnabled_impl() const;

        void setFile_impl(const std::string& filename);
        const std::string& getFileName_impl() const;
        std::atomic<LogLevel> m_logLevel{ LogLevel::Info };
        std::atomic<bool> m_isTesting{ false };
        std::atomic<bool> m_consoleOutputEnabled{ true };
        std::string m_fileName{};
        std::ofstream m_fileStream;
        mutable std::mutex m_mutex;
    };

    template <typename ... Args>
    void Logger::log(const LogLevel level, Fmt<std::type_identity_t<Args>...> fmt, Args&&... args)
    {
        const std::string msg = std::vformat(fmt.value.get(), std::make_format_args(args...));
        instance().log_impl(level, msg, fmt.loc);
    }

    template <typename ... Args>
    void Logger::print(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args)
    {
        const std::string msg = std::vformat(fmt.value.get(), std::make_format_args(args...));
        instance().log_impl(LogLevel::Print, msg, fmt.loc);
    }

    template <typename ... Args>
    void Logger::info(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args)
    {
        const std::string msg = std::vformat(fmt.value.get(), std::make_format_args(args...));
        instance().log_impl(LogLevel::Info, msg, fmt.loc);
    }

    template <typename ... Args>
    void Logger::debug(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args)
    {
        const std::string msg = std::vformat(fmt.value.get(), std::make_format_args(args...));
        instance().log_impl(LogLevel::Debug, msg, fmt.loc);
    }

    template <typename ... Args>
    void Logger::warn(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args)
    {
        const std::string msg = std::vformat(fmt.value.get(), std::make_format_args(args...));
        instance().log_impl(LogLevel::Warning, msg, fmt.loc);
    }

    template <typename ... Args>
    void Logger::error(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args)
    {
        const std::string msg = std::vformat(fmt.value.get(), std::make_format_args(args...));
        instance().log_impl(LogLevel::Error, msg, fmt.loc);
    }

    template <typename ... Args>
    void Logger::fatal(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args)
    {
        const std::string msg = std::vformat(fmt.value.get(), std::make_format_args(args...));
        instance().log_impl(LogLevel::Fatal, msg, fmt.loc);
    }
}

// ReSharper disable CppMemberFunctionMayBeStatic
// Specialize std::formatter for LogLevel
template <>
struct std::formatter<autoinput::LogLevel>
{
    // Parse format specifications (e.g., "{:s}" for string, "{:c}" for char)
    constexpr auto parse(const std::format_parse_context& ctx)  // NOLINT(*-convert-member-functions-to-static)
    {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}')
        {
            if (*it == 'c')
            {
                isShorthand = true;
            }
            ++it;
        }
        return it;
    }

    auto format(const autoinput::LogLevel level, std::format_context& ctx) const // NOLINT(*-convert-member-functions-to-static)
    {
        return std::format_to(ctx.out(), "{}", getLogLevelPrefix(level, isShorthand));
    }

    bool isShorthand{ false };
};
// ReSharper restore CppMemberFunctionMayBeStatic

#endif // INCLUDE_AUTOINPUT_LOGGER_H
