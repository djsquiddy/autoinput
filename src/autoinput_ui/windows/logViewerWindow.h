/**
 * @file logViewerWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_LOG_VIEWER_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_LOG_VIEWER_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "autoinput/support/logger.h"
#include "autoinput/platform/environment.h"
#include <vector>
#include <string>
#include <chrono>

namespace autoinput::ui
{
    /**
     * @brief A window that displays real-time application logs.
     */
    class LogViewerWindow final : public UiWindow
    {
    public:
        explicit LogViewerWindow(const IEnvironment& environment);

    protected:
        void renderContent() override;
        void update() override;
        void onOpen() override;

    private:
        void refreshLogs();
        void clearLogs();
        void copyToClipboard();
        void openLogFile();
        void openLogFolder();

        const IEnvironment& m_environment;
        std::vector<LogEntry> m_logs;
        std::vector<LogEntry> m_filteredLogs;
        
        char m_searchBuffer[256]{ "" };
        int m_selectedLogLevel{ 0 }; // 0: All, 1: Trace, 2: Debug, 3: Info, 4: Warning, 5: Error
        bool m_autoScroll{ true };
        bool m_scrollToBottom{ false };
        std::chrono::steady_clock::time_point m_lastRefreshTime;

        void applyFilters();
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_LOG_VIEWER_WINDOW_H
