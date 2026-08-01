#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "autoinput/platform.h"
#include "autoinput/backend.h"

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
    auto mock = std::make_unique<MockCleanupBackend>();
    MockCleanupBackend* mockPtr = mock.get();
    g_backend = std::move(mock);

    EXPECT_CALL(*mockPtr, cleanup()).Times(Exactly(1));
    
    platform::signalEnd();
    
    g_backend.reset();
}
