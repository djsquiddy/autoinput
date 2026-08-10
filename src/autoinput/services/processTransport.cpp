/**
* @file processTransport.cpp
* @author djsquiddy
* @date August 2026
*/
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "autoinput/services/processTransport.h"
#include "autoinput/support/logger.h"
#include <format>

namespace autoinput::services
{
#ifdef _WIN32
    struct StdioProcessTransport::Impl
    {
        HANDLE childProcess{ nullptr };
        HANDLE childThread{ nullptr };
        HANDLE childStdInWrite{ nullptr };
        HANDLE childStdOutRead{ nullptr };
        std::string lastError;

        ~Impl()
        {
            closeHandles();
        }

        void closeHandles()
        {
            if (childStdInWrite) CloseHandle(childStdInWrite);
            if (childStdOutRead) CloseHandle(childStdOutRead);
            if (childThread) CloseHandle(childThread);
            if (childProcess) CloseHandle(childProcess);

            childStdInWrite = nullptr;
            childStdOutRead = nullptr;
            childThread = nullptr;
            childProcess = nullptr;
        }
    };

    namespace
    {
        std::wstring toWString(std::string_view str)
        {
            if (str.empty()) return L"";
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
            std::wstring wstrTo(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstrTo[0], size_needed);
            return wstrTo;
        }

        std::string quoteWindowsArgumentInternal(const std::string& arg)
        {
            if (arg.empty()) return "\"\"";
            
            bool needsQuotes = arg.find_first_of(" \t\n\v") != std::string::npos;
            if (!needsQuotes && arg.find('"') == std::string::npos) return arg;

            std::string quoted = "\"";
            for (size_t i = 0; i < arg.length(); ++i)
            {
                size_t backslashes = 0;
                while (i < arg.length() && arg[i] == '\\')
                {
                    ++backslashes;
                    ++i;
                }

                if (i == arg.length())
                {
                    // Escape backslashes before the closing quote
                    for (size_t j = 0; j < backslashes * 2; ++j) quoted += '\\';
                    break;
                }
                else if (arg[i] == '"')
                {
                    // Escape backslashes before a double quote, plus the double quote itself
                    for (size_t j = 0; j < backslashes * 2 + 1; ++j) quoted += '\\';
                    quoted += '"';
                }
                else
                {
                    // Backslashes are not special before other characters
                    for (size_t j = 0; j < backslashes; ++j) quoted += '\\';
                    quoted += arg[i];
                }
            }
            quoted += '"';
            return quoted;
        }
    }

    std::string StdioProcessTransport::quoteWindowsArgument(const std::string& arg)
    {
#ifdef _WIN32
        return quoteWindowsArgumentInternal(arg);
#else
        return arg;
#endif
    }
#else
    struct StdioProcessTransport::Impl
    {
        std::string lastError;
    };
#endif

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
        if (m_running) return true;

#ifdef _WIN32
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE hChildStdOutReadTmp = nullptr;
        HANDLE hChildStdOutWrite = nullptr;
        if (!CreatePipe(&hChildStdOutReadTmp, &hChildStdOutWrite, &saAttr, 0))
        {
            m_impl->lastError = "Failed to create stdout pipe.";
            return false;
        }

        if (!SetHandleInformation(hChildStdOutReadTmp, HANDLE_FLAG_INHERIT, 0))
        {
            m_impl->lastError = "Failed to set stdout pipe information.";
            CloseHandle(hChildStdOutReadTmp);
            CloseHandle(hChildStdOutWrite);
            return false;
        }

        HANDLE hChildStdInRead = nullptr;
        HANDLE hChildStdInWriteTmp = nullptr;
        if (!CreatePipe(&hChildStdInRead, &hChildStdInWriteTmp, &saAttr, 0))
        {
            m_impl->lastError = "Failed to create stdin pipe.";
            CloseHandle(hChildStdOutReadTmp);
            CloseHandle(hChildStdOutWrite);
            return false;
        }

        if (!SetHandleInformation(hChildStdInWriteTmp, HANDLE_FLAG_INHERIT, 0))
        {
            m_impl->lastError = "Failed to set stdin pipe information.";
            CloseHandle(hChildStdOutReadTmp);
            CloseHandle(hChildStdOutWrite);
            CloseHandle(hChildStdInRead);
            CloseHandle(hChildStdInWriteTmp);
            return false;
        }

        PROCESS_INFORMATION piProcInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

        STARTUPINFOW siStartInfo;
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFOW));
        siStartInfo.cb = sizeof(STARTUPINFOW);
        siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        siStartInfo.hStdOutput = hChildStdOutWrite;
        siStartInfo.hStdInput = hChildStdInRead;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        std::string commandLine = StdioProcessTransport::quoteWindowsArgument(m_executablePath.string());
        for (const auto& arg : m_arguments)
        {
            commandLine += " ";
            commandLine += StdioProcessTransport::quoteWindowsArgument(arg);
        }

        std::wstring wCommandLine = toWString(commandLine);

        BOOL bSuccess = CreateProcessW(
            NULL,
            wCommandLine.data(),
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            NULL,
            &siStartInfo,
            &piProcInfo
        );

        // Always close child-side pipe handles in the parent
        CloseHandle(hChildStdOutWrite);
        CloseHandle(hChildStdInRead);

        if (!bSuccess)
        {
            m_impl->lastError = std::format("CreateProcessW failed with error code: {}", GetLastError());
            CloseHandle(hChildStdOutReadTmp);
            CloseHandle(hChildStdInWriteTmp);
            return false;
        }

        m_impl->childProcess = piProcInfo.hProcess;
        m_impl->childThread = piProcInfo.hThread;
        m_impl->childStdOutRead = hChildStdOutReadTmp;
        m_impl->childStdInWrite = hChildStdInWriteTmp;
        m_running = true;

        return true;
#else
        m_impl->lastError = "StdioProcessTransport is not implemented for this platform.";
        return false;
#endif
    }

    void StdioProcessTransport::stop()
    {
        if (!m_impl) return;

#ifdef _WIN32
        if (m_impl->childStdInWrite)
        {
            CloseHandle(m_impl->childStdInWrite);
            m_impl->childStdInWrite = nullptr;
        }

        if (m_impl->childProcess)
        {
            // Wait briefly for the child to exit
            if (WaitForSingleObject(m_impl->childProcess, 1000) == WAIT_TIMEOUT)
            {
                TerminateProcess(m_impl->childProcess, 0);
            }
        }
        m_impl->closeHandles();
#endif
        m_running = false;
    }

    bool StdioProcessTransport::running() const
    {
        if (!m_running) return false;

#ifdef _WIN32
        if (m_impl->childProcess)
        {
            DWORD exitCode;
            if (GetExitCodeProcess(m_impl->childProcess, &exitCode))
            {
                if (exitCode != STILL_ACTIVE)
                {
                    const_cast<StdioProcessTransport*>(this)->m_running = false;
                    return false;
                }
            }
        }
#endif
        return m_running;
    }

    std::string StdioProcessTransport::lastError() const
    {
        return m_impl ? m_impl->lastError : "";
    }

    bool StdioProcessTransport::writeLine(std::string_view line)
    {
        if (!running()) return false;

#ifdef _WIN32
        std::string fullLine = std::string(line) + "\n";
        DWORD bytesWritten;
        BOOL bSuccess = WriteFile(
            m_impl->childStdInWrite,
            fullLine.data(),
            static_cast<DWORD>(fullLine.size()),
            &bytesWritten,
            NULL
        );

        if (!bSuccess)
        {
            m_impl->lastError = std::format("WriteFile failed with error code: {}", GetLastError());
            return false;
        }

        if (bytesWritten != fullLine.size())
        {
            m_impl->lastError = std::format("WriteFile only wrote {} of {} bytes.", bytesWritten, fullLine.size());
            return false;
        }

        return true;
#else
        return false;
#endif
    }

    std::optional<std::string> StdioProcessTransport::readLine()
    {
        return readLine(std::chrono::milliseconds(0));
    }

    std::optional<std::string> StdioProcessTransport::readLine(std::chrono::milliseconds timeout)
    {
        if (!running()) return std::nullopt;

#ifdef _WIN32
        std::string line;
        char buffer[1];
        DWORD bytesRead;

        auto startTime = std::chrono::steady_clock::now();

        while (true)
        {
            if (timeout.count() > 0)
            {
                DWORD bytesAvailable = 0;
                if (PeekNamedPipe(m_impl->childStdOutRead, NULL, 0, NULL, &bytesAvailable, NULL))
                {
                    if (bytesAvailable == 0)
                    {
                        auto now = std::chrono::steady_clock::now();
                        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime) >= timeout)
                        {
                            m_impl->lastError = std::format("ReadLine timed out after {}ms.", timeout.count());
                            return std::nullopt;
                        }
                        Sleep(10);
                        continue;
                    }
                }
                else
                {
                    m_impl->lastError = std::format("PeekNamedPipe failed with error code: {}", GetLastError());
                    m_running = false;
                    return std::nullopt;
                }
            }

            BOOL bSuccess = ReadFile(
                m_impl->childStdOutRead,
                buffer,
                1,
                &bytesRead,
                NULL
            );

            if (!bSuccess || bytesRead == 0)
            {
                if (!bSuccess)
                {
                    m_impl->lastError = std::format("ReadFile failed with error code: {}", GetLastError());
                }
                else
                {
                    m_impl->lastError = "ReadFile returned 0 bytes (pipe closed).";
                }
                m_running = false;
                return std::nullopt;
            }

            if (buffer[0] == '\n')
            {
                break;
            }
            
            if (buffer[0] != '\r')
            {
                line += buffer[0];
            }
        }

        return line;
#else
        return std::nullopt;
#endif
    }
}
