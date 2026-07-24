#include "logger.h"
#include "utils.h"


namespace autoinput
{
    LogLevel logLevelFromString(const std::string_view str)
    {
        const std::string level = toLowerCase(str);
        if (level == "d" || level == "debug")
        {
            return LogLevel::Debug;
        }
        if (level == "i" || level == "info")
        {
            return LogLevel::Info;
        }
        if (level == "w" || level == "warning" || level == "warn")
        {
            return LogLevel::Warning;
        }
        if (level == "e" || level == "error")
        {
            return LogLevel::Error;
        }
        if (level == "f" || level == "fatal")
        {
            return LogLevel::Fatal;
        }
        throw std::runtime_error(std::string("Could not parse ") + std::string(str) + " into a valid log level");
    }

    std::string getLogLevelPrefix(const LogLevel level, const bool isShorthand)
    {
        std::string name;
        switch (level)
        {
        case LogLevel::Debug:
            return isShorthand ? "D" : "DEBUG";
        case LogLevel::Print:
            return "";
        case LogLevel::Info:
            return isShorthand ? "I" : "INFO";
        case LogLevel::Warning:
            return isShorthand ? "W" : "WARNING";
        case LogLevel::Error:
            return isShorthand ? "E" : "ERROR";
        case LogLevel::Fatal:
            return isShorthand ? "F" : "Fatal";
        default:
            return isShorthand ? "?" : "UNKNOWN";
        }
        return name;
    }

    std::ostream& getConsoleStream(const LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Error:
        case LogLevel::Fatal:
            return std::cerr;
        case LogLevel::Debug:
        case LogLevel::Print:
        case LogLevel::Info:
        case LogLevel::Warning:
        default:
            return std::cout;
        }
    }

    LogStream::LogStream(Logger& logger, LogLevel level, std::source_location loc)
        : m_logger{ logger }
        , m_level{ level }
        , m_loc{ loc }
    {
    }

    LogStream::~LogStream()
    {
        m_logger.log_impl(m_level, m_buffer.str(), m_loc);
    }

    // Default to console only, or call set_file() early in main()
    Logger::Logger() = default;

    Logger::~Logger()
    {
        if (m_fileStream.is_open())
        {
            m_fileStream.close();
        }
    }

    LogStream Logger::debugStream(std::source_location loc)
    {
        return {instance(), LogLevel::Debug, loc};
    }

    void Logger::debug(const std::string_view msg, const std::source_location loc)
    {
        instance().log_impl(LogLevel::Debug, msg, loc);
    }

    LogStream Logger::infoStream(std::source_location loc)
    {
        return {instance(), LogLevel::Info, loc};
    }

    void Logger::info(const std::string_view msg, const std::source_location loc)
    {
        instance().log_impl(LogLevel::Info, msg, loc);
    }

    LogStream Logger::printStream(const std::source_location loc)
    {
        return {instance(), LogLevel::Print, loc};
    }

    void Logger::print(const std::string_view msg, const std::source_location loc)
    {
        instance().log_impl(LogLevel::Print, msg, loc);
    }

    LogStream Logger::warnStream(std::source_location loc)
    {
        return {instance(), LogLevel::Warning, loc};
    }

    void Logger::warn(const std::string_view msg, const std::source_location loc)
    {
        instance().log_impl(LogLevel::Warning, msg, loc);
    }

    LogStream Logger::errorStream(std::source_location loc)
    {
        return {instance(), LogLevel::Error, loc};
    }

    void Logger::error(const std::string_view msg, const std::source_location loc)
    {
        instance().log_impl(LogLevel::Error, msg, loc);
    }

    LogStream Logger::fatalStream(std::source_location loc)
    {
        return {instance(), LogLevel::Fatal, loc};
    }

    void Logger::fatal(const std::string_view msg, const std::source_location loc)
    {
        instance().log_impl(LogLevel::Fatal, msg, loc);
    }

    void Logger::log_impl(const LogLevel level, const std::string_view msg, const std::source_location loc)
    {
        std::string formatted;
        std::string consoleMsg;
        // Internal implementation logic
        if (level == LogLevel::Print)
        {
            consoleMsg = formatted = msg;
        }
        else
        {
            formatted = std::format("[{}] {} | {}:{}:{} | {}\n",
                level,
                std::format("{:%F %T}", std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now()}),
                std::filesystem::path(loc.file_name()).filename().string(),
                loc.function_name(),
                loc.line(),
                msg
            );
            consoleMsg = std::format("[{}] | {}", level, msg);
        }

        std::scoped_lock lock(m_mutex);
        getConsoleStream(level) << consoleMsg;
        if (m_fileStream.is_open())
        {
            m_fileStream << formatted;
            m_fileStream.flush();
        }
    }

    void Logger::setLogLevel(const LogLevel logLevel)
    {
        instance().setLogLevel_impl(logLevel);
    }

    void Logger::setLogLevel_impl(const LogLevel logLevel)
    {
        std::scoped_lock lock(m_mutex);
        m_logLevel = logLevel;
    }

    void Logger::flush()
    {
        instance().flush_impl();
    }

    void Logger::flush_impl()
    {
        std::scoped_lock lock(m_mutex);
        if (m_fileStream.is_open())
        {
            m_fileStream.flush();
        }
    }

    void Logger::setFile(const std::string& filename)
    {
        instance().setFile_impl(filename);
    }

    void Logger::setFile_impl(const std::string& filename)
    {
        std::scoped_lock lock(m_mutex);
        if (m_fileStream.is_open())
        {
            m_fileStream.close();
        }
        m_fileStream.open(filename, std::ios::app);
        if (!m_fileStream.is_open())
        {
            throw std::runtime_error("Failed to open log file: " + filename);
        }
    }
}