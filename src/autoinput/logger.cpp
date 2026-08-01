/**
 * @file logger.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/logger.h"
#include "autoinput/utils.h"


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
        return LogLevel::Unknown;
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

    // Default to console only, or call setFile()
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

    Logger& Logger::instance()
    {
        static Logger instance;
        return instance;
    }

    void Logger::log(const LogLevel level, const std::string_view msg, const std::source_location loc)
    {
        instance().log_impl(level, msg, loc);
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

    bool Logger::isDebugModeEnabled()
    {
        using logLevel_t = std::underlying_type_t<LogLevel>;
        return static_cast<logLevel_t>(getLogLevel()) >= static_cast<logLevel_t>(LogLevel::Debug);
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
            using logLevel_t = std::underlying_type_t<LogLevel>;
            if (static_cast<logLevel_t>(getLogLevel_impl()) > static_cast<logLevel_t>(level))
            {
                return;
            }

            std::string timeStr;
            try {
                timeStr = std::format("{:%F %T}", std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now()});
            } catch (...) {
                timeStr = "UNKNOWN TIME";
            }

            formatted = std::format("[{}] {} | {}:{}:{} | {}\n",
                level,
                timeStr,
                std::filesystem::path(loc.file_name()).filename().string(),
                loc.function_name(),
                loc.line(),
                msg
            );
            consoleMsg = std::format("[{}] | {}", level, msg);
        }

        if (isTesting_impl())
        {
            return;
        }

        std::scoped_lock lock(m_mutex);
        getConsoleStream(level) << consoleMsg;
        if (m_fileStream.is_open())
        {
            m_fileStream << formatted;
            if (level >= LogLevel::Warning)
            {
                m_fileStream.flush();
            }
        }
    }

    void Logger::setLogLevel(const LogLevel logLevel)
    {
        instance().setLogLevel_impl(logLevel);
    }

    void Logger::setLogLevel_impl(const LogLevel logLevel)
    {
        m_logLevel = logLevel;
    }

    LogLevel Logger::getLogLevel()
    {
        return instance().getLogLevel_impl();
    }

    LogLevel Logger::getLogLevel_impl() const
    {
        return m_logLevel.load();
    }

    void Logger::setTesting(const bool testing)
    {
        instance().setTesting_impl(testing);
    }

    bool Logger::isTesting()
    {
        return instance().isTesting_impl();
    }

    void Logger::setTesting_impl(const bool testing)
    {
        m_isTesting = testing;
    }

    bool Logger::isTesting_impl() const
    {
        return m_isTesting.load();
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

    const std::string& Logger::getFileName()
    {
        return instance().getFileName_impl();
    }

    const std::string& Logger::getFileName_impl() const
    {
        std::scoped_lock lock(m_mutex);
        return m_fileName;
    }
}
