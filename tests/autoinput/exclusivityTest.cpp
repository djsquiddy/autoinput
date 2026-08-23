/**
 * @file exclusivityTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/app/autoinput.h"
#include "autoinput/platform/backend.h"
#include "testUtils.h"

namespace autoinput
{
    class ExclusivityTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            auto backend = std::make_unique<FakeBackend>();
            m_program = std::make_unique<Program>(std::move(backend));
            m_program->arguments().jsonOutput = true;
        }

        std::unique_ptr<Program> m_program;
    };

    TEST_F(ExclusivityTest, ExclusiveGroupBehavior)
    {
        ProgramArguments& args = m_program->arguments();
        args.delayData.minWaitPressDelay = std::chrono::milliseconds(10);
        args.delayData.maxWaitPressDelay = std::chrono::milliseconds(10);
        args.delayData.minWaitReleaseDelay = std::chrono::milliseconds(10);
        args.delayData.maxWaitReleaseDelay = std::chrono::milliseconds(10);
        
        // Command 1: left click, trigger back, group G1
        args.buttons.push_back(Mouse(MouseButton::Left));
        args.startKeys.push_back("back");
        args.targetActions.push_back(ActionState::CLICK);
        args.commandNames.push_back("cmd1");
        args.exclusiveGroups.push_back("G1");

        // Command 2: right click, trigger forward, group G1
        args.buttons.push_back(Mouse(MouseButton::Right));
        args.startKeys.push_back("forward");
        args.targetActions.push_back(ActionState::CLICK);
        args.commandNames.push_back("cmd2");
        args.exclusiveGroups.push_back("G1");

        // Command 3: space key, trigger f6, no group
        args.keys.push_back(Key{.character = "space"});
        args.startKeys.push_back("f6");
        args.targetActions.push_back(ActionState::CLICK);
        args.commandNames.push_back("cmd3");
        args.exclusiveGroups.push_back("");

        // Ensure program initializes successfully
        ASSERT_TRUE(m_program->init());

        auto& mouseHandlers = m_program->getMouseHandlers();
        auto& keyHandlers = m_program->getKeyHandlers();
        
        Mouse leftMouse(MouseButton::Left);
        Mouse rightMouse(MouseButton::Right);
        Key spaceKey{.character = "space"};

        // 1. Start cmd1
        const auto& keyInfo = m_program->getKeyInfo();
        m_program->start(keyInfo[0]); // cmd1
        // Verify only cmd1 left mouse handler becomes active
        EXPECT_TRUE(mouseHandlers[leftMouse].getActive());
        EXPECT_FALSE(mouseHandlers[rightMouse].getActive());
        EXPECT_FALSE(keyHandlers[spaceKey].getActive());

        // 2. Start cmd2 -> should stop cmd1
        m_program->start(keyInfo[1]); // cmd2
        // Ensure starting cmd2 stops cmd1 due to shared exclusive group G1
        EXPECT_FALSE(mouseHandlers[leftMouse].getActive());
        EXPECT_TRUE(mouseHandlers[rightMouse].getActive());
        EXPECT_FALSE(keyHandlers[spaceKey].getActive());

        // 3. Start cmd3 -> should NOT stop cmd2
        m_program->start(keyInfo[2]); // cmd3
        // Ensure starting un-grouped cmd3 does not affect active cmd2
        EXPECT_FALSE(mouseHandlers[leftMouse].getActive());
        EXPECT_TRUE(mouseHandlers[rightMouse].getActive());
        EXPECT_TRUE(keyHandlers[spaceKey].getActive());

        // 4. Start cmd1 again -> should stop cmd2, but NOT cmd3
        m_program->start(keyInfo[0]); // cmd1
        // Ensure starting cmd1 stops conflicting cmd2 while leaving un-grouped cmd3 active
        EXPECT_TRUE(mouseHandlers[leftMouse].getActive());
        EXPECT_FALSE(mouseHandlers[rightMouse].getActive());
        EXPECT_TRUE(keyHandlers[spaceKey].getActive());

        // 5. Toggle cmd1 off
        m_program->start(keyInfo[0]); // cmd1 toggle off
        // Ensure toggling off cmd1 leaves only cmd3 running
        EXPECT_FALSE(mouseHandlers[leftMouse].getActive());
        EXPECT_FALSE(mouseHandlers[rightMouse].getActive());
        EXPECT_TRUE(keyHandlers[spaceKey].getActive());
        
        m_program->end();
    }

    TEST_F(ExclusivityTest, DifferentGroupsAreIndependent)
    {
        ProgramArguments& args = m_program->arguments();
        args.delayData.minWaitPressDelay = std::chrono::milliseconds(10);
        args.delayData.maxWaitPressDelay = std::chrono::milliseconds(10);
        args.delayData.minWaitReleaseDelay = std::chrono::milliseconds(10);
        args.delayData.maxWaitReleaseDelay = std::chrono::milliseconds(10);
        
        // Command 1: left click, trigger back, group G1
        args.buttons.push_back(Mouse(MouseButton::Left));
        args.startKeys.push_back("back");
        args.targetActions.push_back(ActionState::CLICK);
        args.commandNames.push_back("cmd1");
        args.exclusiveGroups.push_back("G1");

        // Command 2: right click, trigger forward, group G2
        args.buttons.push_back(Mouse(MouseButton::Right));
        args.startKeys.push_back("forward");
        args.targetActions.push_back(ActionState::CLICK);
        args.commandNames.push_back("cmd2");
        args.exclusiveGroups.push_back("G2");

        // Ensure program initializes successfully
        ASSERT_TRUE(m_program->init());

        auto& mouseHandlers = m_program->getMouseHandlers();
        Mouse leftMouse(MouseButton::Left);
        Mouse rightMouse(MouseButton::Right);

        const auto& keyInfo = m_program->getKeyInfo();
        
        m_program->start(keyInfo[0]); // cmd1
        // Verify cmd1 starts successfully
        EXPECT_TRUE(mouseHandlers[leftMouse].getActive());
        
        m_program->start(keyInfo[1]); // cmd2
        // Ensure starting cmd2 from different group G2 runs concurrently with cmd1 in G1
        EXPECT_TRUE(mouseHandlers[leftMouse].getActive());
        EXPECT_TRUE(mouseHandlers[rightMouse].getActive());
        
        m_program->end();
    }
}
