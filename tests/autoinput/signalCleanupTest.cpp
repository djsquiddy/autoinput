/**
 * @file signalCleanupTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "autoinput/autoInput.h"
#include "autoinput/platform.h"
#include "autoinput/backend.h"
#include "testUtils.h"

using namespace autoinput;
using ::testing::Exactly;

class MockCleanupBackend : public IPlatformBackend
{
public:
    MOCK_METHOD(bool, installHooks, (), (override));
    MOCK_METHOD(void, runListener, (), (override));
    MOCK_METHOD(void, cleanup, (), (override));
    MOCK_METHOD(void, keyPress, (const Key& key), (override));
    MOCK_METHOD(void, keyRelease, (const Key& key), (override));
    MOCK_METHOD(void, mousePress, (const Mouse& mouse), (override));
    MOCK_METHOD(void, mouseRelease, (const Mouse& mouse), (override));
};

TEST(SignalCleanupTest, SignalEndCallsCleanup)
{
    g_program = std::make_unique<Program>();
    auto mock = std::make_unique<MockCleanupBackend>();
    MockCleanupBackend* mockPtr = mock.get();
    test::ScopedBackendOverride backendOverride(std::move(mock));

    EXPECT_CALL(*mockPtr, cleanup()).Times(Exactly(1));
    
    platform::signalEnd();

    g_program.reset();
}
