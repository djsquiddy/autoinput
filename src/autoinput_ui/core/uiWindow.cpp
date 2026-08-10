/**
 * @file uiWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "uiWindow.h"
#include "localization.h"
#include "../widgets/modalWidgets.h"
#include <imgui.h>

namespace autoinput::ui
{
    UiWindow::UiWindow(std::string title, std::string titleKey)
        : m_title(std::move(title)), m_titleKey(std::move(titleKey))
    {
    }

    void UiWindow::open()
    {
        if (!m_isOpen)
        {
            m_isOpen = true;
            m_shouldFocus = true;
            onOpen();
        }
    }

    void UiWindow::requestClose()
    {
        if (m_isDirty)
        {
            m_showSaveConfirmation = true;
        }
        else
        {
            m_isOpen = false;
            onCloseConfirmed();
        }
    }

    void UiWindow::render()
    {
        if (!m_isOpen) return;

        if (m_isFullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
        }

        if (m_shouldFocus)
        {
            ImGui::SetNextWindowFocus();
            m_shouldFocus = false;
        }

        bool open = m_isOpen;
        bool* p_open = hasCloseButton() ? &open : nullptr;

        std::string title = m_title;
        if (!m_titleKey.empty())
        {
            title = Localization::get().textOr(m_titleKey, m_title);
        }

        if (ImGui::Begin(title.c_str(), p_open, getFlags()))
        {
            renderContent();
        }
        ImGui::End();

        if (hasCloseButton() && !open)
        {
            requestClose();
        }

        if (m_showSaveConfirmation)
        {
            renderSaveConfirmation();
        }
    }

    bool UiWindow::isOpen() const
    {
        return m_isOpen;
    }

    bool UiWindow::isDirty() const
    {
        return m_isDirty;
    }

    void UiWindow::discardChanges()
    {
        clearDirty();
        m_isOpen = false;
        onCloseConfirmed();
    }

    void UiWindow::renderSaveConfirmation()
    {
        // ReSharper disable once CppTooWideScopeInitStatement
        const widgets::SaveConfirmationResult res = widgets::RenderSaveConfirmationModal("Unsaved Changes?", m_title);

        if (res == widgets::SaveConfirmationResult::Save)
        {
            save();
            clearDirty();
            m_isOpen = false;
            m_showSaveConfirmation = false;
            onCloseConfirmed();
        }
        else if (res == widgets::SaveConfirmationResult::Discard)
        {
            discardChanges();
            m_showSaveConfirmation = false;
        }
        else if (res == widgets::SaveConfirmationResult::Cancel)
        {
            m_showSaveConfirmation = false;
        }
    }

    void UiWindow::markDirty()
    {
        m_isDirty = true;
    }

    void UiWindow::clearDirty()
    {
        m_isDirty = false;
    }
}
