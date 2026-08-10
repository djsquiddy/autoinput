/**
 * @file logger.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_SUPPORT_LOGGER_H
#define INCLUDE_AUTOINPUT_SUPPORT_LOGGER_H
#pragma once

#include <cstdlib> // For std::abort
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

#include "autoinput/support/types.h"

namespace autoinput
{
    struct ErrorMessage;
    enum class ErrorCode : i32;
    class Logger;

    enum class LogLevel : uint8_t
    {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Print = 3,
        Warning = 4,
        Error = 5,
        Fatal = 6,
        Unknown = 7
    };

    /**
     * @brief Represents a single log entry for in-memory storage.
     */
    struct LogEntry
    {
        LogLevel level;
        std::string message;
        std::string timestamp;
        std::string file;
        std::string function;
        int line;
    };

    /**
     * @brief Converts a string to a LogLevel.
     * @param str The string representation of the log level.
     * @return The corresponding LogLevel.
     */
    LogLevel logLevelFromString(std::string_view str);

    /**
     * @brief Converts a LogLevel to a string.
     * @param level The log level.
     * @return The string representation of the log level.
     */
    std::string logLevelToString(LogLevel level);

    /**
     * @brief Gets the prefix string for a log level.
     * @param level The log level.
     * @param isShorthand Whether to use a shorthand prefix (e.g. [I] instead of [INFO]).
     * @param isColored Whether to use colored output.
     * @return The prefix string.
     */
    std::string getLogLevelPrefix(LogLevel level, bool isShorthand = false, bool isColored = false);

    // Overload the insertion operator
    inline std::ostream& operator<<(std::ostream& os, const LogLevel level) { return os << getLogLevelPrefix(level); }

    /**
     * @brief Gets the appropriate console stream for a log level (cout or cerr).
     * @param level The log level.
     * @return Reference to the ostream.
     */
    std::ostream& getConsoleStream(LogLevel level);

    // Helper class to enable: Logger::info << "msg";
    class LogStream
    {
    public:
        /**
         * @brief Constructs a LogStream.
         * @param logger Reference to the Logger.
         * @param level The log level for this stream.
         * @param loc The source location where the stream was created.
         */
        LogStream(Logger& logger, LogLevel level, std::source_location loc);

        /**
         * @brief Destructor that flushes the buffered log message to the logger.
         */
        ~LogStream();

        /**
         * @brief Template operator<< to accept any streamable type.
         * @tparam T The type of the value to stream.
         * @param value The value to add to the buffer.
         * @return Reference to this LogStream.
         */
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

        /**
         * @brief Gets the singleton instance of the Logger.
         * @return Reference to the Logger instance.
         */
        static Logger& instance();

        /**
         * @brief Logs a message with a specific log level and source location.
         * @param level The log level.
         * @param msg The message to log.
         * @param loc The source location.
         */
        static void log(LogLevel level, std::string_view msg, std::source_location loc = std::source_location::current());

        /**
         * @brief Logs a formatted message.
         * @tparam Args The types of the format arguments.
         * @param level The log level.
         * @param fmt The format string and captured source location.
         * @param args The format arguments.
         */
        template <typename... Args>
        static void log(LogLevel level, Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        /**
         * @brief Logs a message at Print level.
         * @param msg The message.
         * @param loc The source location.
         */
        static void print(std::string_view msg, std::source_location loc = std::source_location::current());
        /**
         * @brief Logs a formatted message at Print level.
         */
        template <typename... Args>
        static void print(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        /**
         * @brief Logs a message at Trace level.
         */
        static void trace(std::string_view msg, std::source_location loc = std::source_location::current());
        /**
         * @brief Logs a formatted message at Trace level.
         */
        template <typename... Args>
        static void trace(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        /**
         * @brief Logs a message at Info level.
         */
        static void info(std::string_view msg, std::source_location loc = std::source_location::current());
        /**
         * @brief Logs a formatted message at Info level.
         */
        template <typename... Args>
        static void info(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        /**
         * @brief Logs a message at Debug level.
         * @param msg The message.
         * @param loc The source location.
         */
        static void debug(std::string_view msg, std::source_location loc = std::source_location::current());
        /**
         * @brief Logs a formatted message at Debug level.
         */
        template <typename... Args>
        static void debug(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        /**
         * @brief Logs a message at Warning level.
         * @param msg The message.
         * @param loc The source location.
         */
        static void warn(std::string_view msg, std::source_location loc = std::source_location::current());
        /**
         * @brief Logs a formatted message at Warning level.
         */
        template <typename... Args>
        static void warn(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        /**
         * @brief Logs a message at Error level.
         * @param msg The message.
         * @param loc The source location.
         */
        static void error(std::string_view msg, std::source_location loc = std::source_location::current());
        /**
         * @brief Logs a formatted message at Error level.
         */
        template <typename... Args>
        static void error(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        /**
         * @brief Logs a message at Fatal level.
         * @param msg The message.
         * @param loc The source location.
         */
        static void fatal(std::string_view msg, std::source_location loc = std::source_location::current());
        /**
         * @brief Logs a formatted message at Fatal level.
         */
        template <typename... Args>
        static void fatal(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args);

        /**
         * @brief Logs a fatal error message and returns the error code.
         * @param error The error message data.
         * @return The ErrorCode.
         */
        static ErrorCode fatalError(const ErrorMessage& error);
        
        /**
         * @brief Logs multiple fatal errors.
         * @param errors The vector of error messages.
         */
        static void fatalError(const std::vector<ErrorMessage>& errors);

        /**
         * @brief Creates a LogStream for Trace level.
         * @param loc The source location.
         * @return The LogStream.
         */
        static LogStream traceStream(std::source_location loc = std::source_location::current());
        /**
         * @brief Creates a LogStream for Debug level.
         * @param loc The source location.
         * @return The LogStream.
         */
        static LogStream debugStream(std::source_location loc = std::source_location::current());
        /**
         * @brief Creates a LogStream for Print level.
         * @param loc The source location.
         * @return The LogStream.
         */
        static LogStream printStream(std::source_location loc = std::source_location::current());
        /**
         * @brief Creates a LogStream for Info level.
         * @param loc The source location.
         * @return The LogStream.
         */
        static LogStream infoStream(std::source_location loc = std::source_location::current());
        /**
         * @brief Creates a LogStream for Warning level.
         * @param loc The source location.
         * @return The LogStream.
         */
        static LogStream warnStream(std::source_location loc = std::source_location::current());
        /**
         * @brief Creates a LogStream for Error level.
         * @param loc The source location.
         * @return The LogStream.
         */
        static LogStream errorStream(std::source_location loc = std::source_location::current());
        /**
         * @brief Creates a LogStream for Fatal level.
         * @param loc The source location.
         * @return The LogStream.
         */
        static LogStream fatalStream(std::source_location loc = std::source_location::current());

        /**
         * @brief Checks if debug mode is enabled.
         * @return True if debug level is set.
         */
        static bool isDebugModeEnabled();

        /**
         * @brief Sets the current log level.
         * @param logLevel The new log level.
         */
        static void setLogLevel(LogLevel logLevel);
        
        /**
         * @brief Gets the current log level.
         * @return The current log level.
         */
        static LogLevel getLogLevel();
        
        /**
         * @brief Flushes the logger output.
         */
        static void flush();
        
        /**
         * @brief Sets whether the logger is in testing mode.
         * @param testing True to enable testing mode.
         */
        static void setTesting(bool testing);
        
        /**
         * @brief Checks if the logger is in testing mode.
         * @return True if in testing mode.
         */
        static bool isTesting();
        
        /**
         * @brief Sets whether console output is enabled.
         * @param enabled True to enable console output.
         */
        static void setConsoleOutputEnabled(bool enabled);
        
        /**
         * @brief Checks if console output is enabled.
         * @return True if console output is enabled.
         */
        static bool isConsoleOutputEnabled();
        
        /**
         * @brief Sets the file path for logging.
         * @param filename The path to the log file.
         */
        static void setFile(const std::string& filename);
        
        /**
         * @brief Gets the current log file name.
         * @return The file name.
         */
        static const std::string& getFileName();

        /**
         * @brief Gets recent log entries stored in memory.
         * @return A vector of LogEntry objects.
         */
        static std::vector<LogEntry> getRecentLogs();

        /**
         * @brief Clears the in-memory log entries.
         */
        static void clearRecentLogs();

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

        std::vector<LogEntry> getRecentLogs_impl() const;
        void clearRecentLogs_impl();

        std::atomic<LogLevel> m_logLevel{ LogLevel::Info };
        std::atomic<bool> m_isTesting{ false };
        std::atomic<bool> m_consoleOutputEnabled{ true };
        std::string m_fileName{};
        std::ofstream m_fileStream;
        std::vector<LogEntry> m_recentLogs;
        static constexpr size_t MAX_RECENT_LOGS = 1000;
        mutable std::mutex m_mutex;
    };

    template <typename ... Args>
    void Logger::trace(Fmt<std::type_identity_t<Args>...> fmt, Args&&... args)
    {
        const std::string msg = std::vformat(fmt.value.get(), std::make_format_args(args...));
        instance().log_impl(LogLevel::Trace, msg, fmt.loc);
    }

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
        while (it != ctx.end() && *it != '}')
        {
            if (*it == 'c')
            {
                isShorthand = true;
            }
            if (*it == 'C')
            {
                isColored = true;
            }
            ++it;
        }
        return it;
    }

    auto format(const autoinput::LogLevel level, std::format_context& ctx) const // NOLINT(*-convert-member-functions-to-static)
    {
        return std::format_to(ctx.out(), "{}", getLogLevelPrefix(level, isShorthand, isColored));
    }

    bool isShorthand{ false };
    bool isColored { false };
};
// ReSharper restore CppMemberFunctionMayBeStatic

#ifndef NDEBUG
/**
 * @brief Custom assertion that logs a fatal error and aborts.
 * Supports optional format strings and arguments.
 * Example: AUTOINPUT_ASSERT(x > 0, "Value must be positive, but is {}", x);
 */
#define AUTOINPUT_ASSERT(condition, ...) \
        do { \
            if (!(condition)) { \
                autoinput::Logger::fatal("Assertion failed: (" #condition ") " __VA_ARGS__); \
                std::abort(); \
            } \
        } while (false)
#else
#define AUTOINPUT_ASSERT(condition, ...) ((void)0)
#endif

#endif // INCLUDE_AUTOINPUT_SUPPORT_LOGGER_H
