/**
 * @file config.cpp
 * @author djsquiddy
 * @date July 2026
 */

#ifndef INCLUDE_AUTOINPUT_LOGGER_H
#define INCLUDE_AUTOINPUT_LOGGER_H
#pragma once
#include <string_view>
#include <chrono>
#include <format>
#include <fstream>
#include <mutex>
#include <source_location>

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
    inline std::ostream& getConsoleStream(LogLevel level);

    // Helper class to enable: Logger::info << "msg";
    class LogStream
    {
    public:
        LogStream(Logger& logger, LogLevel level, std::source_location loc);

        // Capture the destructor to flush the log (optional, or flush on every <<)
        ~LogStream();

        // Template operator<< to accept any streamable type
        template <typename T>
        LogStream& operator<<(const T& value) {
            m_buffer << value;
            return *this;
        }

    private:
        Logger& m_logger;
        LogLevel m_level;
        std::source_location m_loc;
        std::ostringstream m_buffer;
    };

    class Logger
    {
    public:
        // Delete copy and move operations to enforce singleton
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        static Logger& instance()
        {
            static Logger instance;
            return instance;
        }

        // Generic log method with format arguments
        static void log(const LogLevel level, const std::string_view msg, const std::source_location loc = std::source_location::current())
        {
            instance().log_impl(level, msg, loc);
        }
        template <typename... Args>
        static void log(const LogLevel level, std::format_string<Args...> fmt, Args&&... args)
        {
            const std::string msg = std::format(fmt, std::forward<Args>(args)...);
            instance().log_impl(level, msg, std::source_location::current());
        }

        // Convenience wrappers for specific levels
        static void print(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void print(std::format_string<Args...> fmt, Args&&... args)
        {
            const std::string msg = std::format(fmt, std::forward<Args>(args)...);
            instance().log_impl(LogLevel::Print, msg, std::source_location::current());
        }
        static void info(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void info(std::format_string<Args...> fmt, Args&&... args)
        {
            const std::string msg = std::format(fmt, std::forward<Args>(args)...);
            instance().log_impl(LogLevel::Info, msg, std::source_location::current());
        }

        static void debug(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void debug(std::format_string<Args...> fmt, Args&&... args)
        {
            const std::string msg = std::format(fmt, std::forward<Args>(args)...);
            instance().log_impl(LogLevel::Debug, msg, std::source_location::current());
        }

        static void warn(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void warn(std::format_string<Args...> fmt, Args&&... args)
        {
            const std::string msg = std::format(fmt, std::forward<Args>(args)...);
            instance().log_impl(LogLevel::Warning, msg, std::source_location::current());
        }

        static void error(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void error(std::format_string<Args...> fmt, Args&&... args)
        {
            const std::string msg = std::format(fmt, std::forward<Args>(args)...);
            instance().log_impl(LogLevel::Error, msg, std::source_location::current());
        }

        static void fatal(std::string_view msg, std::source_location loc = std::source_location::current());
        template <typename... Args>
        static void fatal(std::format_string<Args...> fmt, Args&&... args)
        {
            const std::string msg = std::format(fmt, std::forward<Args>(args)...);
            instance().log_impl(LogLevel::Fatal, msg, std::source_location::current());
        }

        static LogStream debugStream(std::source_location loc = std::source_location::current());
        static LogStream printStream(std::source_location loc = std::source_location::current());
        static LogStream infoStream(std::source_location loc = std::source_location::current());
        static LogStream warnStream(std::source_location loc = std::source_location::current());
        static LogStream errorStream(std::source_location loc = std::source_location::current());
        static LogStream fatalStream(std::source_location loc = std::source_location::current());

        static void setLogLevel(LogLevel logLevel);
        static void flush();
        // Optional: Allow configuring the file path at runtime
        static void setFile(const std::string& filename);

    private:
        friend class LogStream;
        // Private constructor
        Logger();

        ~Logger();

        void log_impl(LogLevel level, std::string_view msg, std::source_location loc = std::source_location::current());
        void flush_impl();
        void setLogLevel_impl(LogLevel logLevel);
        void setFile_impl(const std::string& filename);
        LogLevel m_logLevel{ LogLevel::Debug };
        std::ofstream m_fileStream;
        std::mutex m_mutex;
        std::once_flag m_onceFlag;
    };
}

// ReSharper disable CppMemberFunctionMayBeStatic
// Specialize std::formatter for LogLevel
template <>
struct std::formatter<autoinput::LogLevel>
{
    // Parse format specifications (e.g., "{:s}" for string, "{:c}" for char)
    constexpr auto parse(std::format_parse_context& ctx)  // NOLINT(*-convert-member-functions-to-static)
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
