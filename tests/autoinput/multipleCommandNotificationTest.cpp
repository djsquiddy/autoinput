/**
 * @file multipleCommandNotificationTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/autoinput.h"
#include "autoinput/notifications.h"
#include "autoinput/backend.h"
#include "autoinput/mouse.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace autoinput;
using ::testing::_;

namespace autoinput::testing
{
    class MockNotificationSink : public INotificationSink
    {
    public:
        MOCK_METHOD(void, notify, (const std::string& title, const std::string& body), (override));
    };

    class ReproNotificationTest : public ::testing::Test
    {
    protected:
        std::unique_ptr<Program> m_testProgram;
        void SetUp() override
        {
            m_testProgram = std::make_unique<Program>();
            m_testProgram->setBackend(std::make_unique<FakeBackend>());
            g_program = m_testProgram.get();
        }

        void TearDown() override
        {
            if (g_program)
            {
                g_program->end();
            }
            g_program = nullptr;
            m_testProgram.reset();
        }
    };

    TEST_F(ReproNotificationTest, MultipleCommandsTriggerNotifications)
    {
        g_program->arguments().buttons = {MouseButton::Left, MouseButton::Right};
        g_program->arguments().commandNames = {"LeftClick", "RightClick"};
        g_program->arguments().startKeys = {"f1", "f2"}; // triggers
        g_program->arguments().statusNotificationMode = StatusNotificationMode::Both;
        ASSERT_TRUE(g_program->init());

        auto mockSink = std::make_unique<MockNotificationSink>();
        auto* mockSinkPtr = mockSink.get();
        g_program->getNotificationService()->addSink(std::move(mockSink));

        // Find KeyInfo for f1 and f2
        const KeyInfo* f1Info = nullptr;
        const KeyInfo* f2Info = nullptr;
        for (const auto& info : g_program->getKeyInfo())
        {
            if (static_cast<bool>(info.triggerKey.modifier & KeyModifier::Function))
            {
                if (info.triggerKey.character == "1") f1Info = &info;
                if (info.triggerKey.character == "2") f2Info = &info;
            }
        }

        ASSERT_NE(f1Info, nullptr);
        ASSERT_NE(f2Info, nullptr);

        {
            ::testing::InSequence s;
            EXPECT_CALL(*mockSinkPtr, notify(_, "Auto clicking (LeftClick): ACTIVE")).Times(1);
            EXPECT_CALL(*mockSinkPtr, notify(_, "Auto clicking (RightClick): ACTIVE")).Times(1);
            // Verify that stopping one command while another is active results in a PAUSED notification for that command
            EXPECT_CALL(*mockSinkPtr, notify(_, "Auto clicking (LeftClick): PAUSED")).Times(1);
            // Expect final PAUSED notification from Program::end() in TearDown
            EXPECT_CALL(*mockSinkPtr, notify(_, "Auto clicking: PAUSED")).Times(1);
        }

        // Activate first command via start()
        g_program->start(*f1Info);

        // Activate second command via start()
        g_program->start(*f2Info);

        // Deactivate first command via start() (toggles)
        g_program->start(*f1Info);
    }
}
