#include <gtest/gtest.h>
#include "autoinput/logger.h"

namespace autoinput
{
    class TestEnvironment : public ::testing::Environment
    {
    public:
        void SetUp() override
        {
            Logger::setTesting(true);
        }
    };

    ::testing::Environment* const testEnv = ::testing::AddGlobalTestEnvironment(new TestEnvironment);
}
