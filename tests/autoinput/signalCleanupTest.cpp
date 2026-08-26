/**
 * @file signalCleanupTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/app/autoinput.h"
#include "autoinput/platform/platform.h"
#include "autoinput/platform/backend.h"
#include "testUtils.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace autoinput;
using ::testing::Exactly;

class MockCleanupBackend : public IPlatformBackend
{
public:
    MOCK_METHOD(bool, installHooks, (), (override));
    MOCK_METHOD(void, runListener, (), (override));
    MOCK_METHOD(void, cleanup, (), (override));
    MOCK_METHOD(void, requestStop, (), (override));
    MOCK_METHOD(void, keyPress, (const Key& key), (override));
    MOCK_METHOD(void, keyRelease, (const Key& key), (override));
    MOCK_METHOD(void, mousePress, (const Mouse& mouse), (override));
    MOCK_METHOD(void, mouseRelease, (const Mouse& mouse), (override));
    MOCK_METHOD(void, keyDown, (const Key& key), (override));
    MOCK_METHOD(void, keyUp, (const Key& key), (override));
    MOCK_METHOD(void, mouseDown, (const Mouse& mouse), (override));
    MOCK_METHOD(void, mouseUp, (const Mouse& mouse), (override));
    MOCK_METHOD(void, moveMouseTo, (int32_t x, int32_t y), (override));
    MOCK_METHOD((std::pair<int32_t, int32_t>), getCursorPosition, (), (override));
    MOCK_METHOD(std::vector<AppWindowInfo>, enumerateWindows, (), (override));
    std::optional<AppWindowInfo> getForegroundWindow() override { return std::nullopt; }
    BackendCapabilities capabilities() const override { return {}; }
    std::string getName() const override { return "Mock Cleanup Backend"; }
};

TEST(SignalCleanupTest, SignalEndCallsCleanup)
{
    auto testProgram = std::make_unique<Program>();
    auto mock = std::make_unique<MockCleanupBackend>();
    MockCleanupBackend* mockPtr = mock.get();
    testProgram->setBackend(std::move(mock));
    g_program = testProgram.get();
    
    // Verify cleanup is invoked exactly once on the platform backend when signaling program end
    EXPECT_CALL(*mockPtr, cleanup()).Times(Exactly(1));
    
    platform::signalEnd();

    g_program = nullptr;
    testProgram.reset();
}
