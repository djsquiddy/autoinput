/**
 * @file processTransportIntegrationTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/services/processTransport.h"
#include "autoinput/services/runtimeProtocol.h"
#include "autoinput/services/automationRuntimeClient.h"
#include <filesystem>
#include <vector>
#include <string>

namespace
{
    std::filesystem::path autoinputExePath()
    {
#ifdef AUTOINPUT_EXE_PATH
        return std::filesystem::path{ AUTOINPUT_EXE_PATH };
#else
        return {};
#endif
    }

    bool canRunAutoinputExe()
    {
        const auto path = autoinputExePath();
        return !path.empty() && std::filesystem::exists(path);
    }
}

namespace autoinput::services
{
    TEST(ProcessTransportIntegrationTest, StdioTransportCanSendStatusAndShutdown)
    {
        if (!canRunAutoinputExe())
        {
            GTEST_SKIP() << "AUTOINPUT_EXE_PATH is not available.";
        }

        autoinput::services::StdioProcessTransport transport(
            autoinputExePath(),
            std::vector<std::string>{ "serve", "--stdio" }
        );

        ASSERT_TRUE(transport.start()) << transport.lastError();

        ASSERT_TRUE(transport.writeLine("{\"id\":1,\"method\":\"status\"}"));
        auto response = transport.readLine(std::chrono::milliseconds(5000));
        ASSERT_TRUE(response.has_value()) << "Timeout reading status response";

        auto result = autoinput::services::parseRuntimeResponse(*response);
        EXPECT_TRUE(result.success);
        EXPECT_EQ(result.status, autoinput::services::RuntimeStatus::Stopped);
        EXPECT_EQ(result.message, "Status retrieved.");

        ASSERT_TRUE(transport.writeLine("{\"id\":2,\"method\":\"shutdown\"}"));
        response = transport.readLine(std::chrono::milliseconds(5000));
        ASSERT_TRUE(response.has_value()) << "Timeout reading shutdown response";

        result = autoinput::services::parseRuntimeResponse(*response);
        EXPECT_TRUE(result.success);
        EXPECT_EQ(result.status, autoinput::services::RuntimeStatus::Stopped);

        transport.stop();
    }

    TEST(ProcessTransportIntegrationTest, StdioTransportCanSendStop)
    {
        if (!canRunAutoinputExe())
        {
            GTEST_SKIP() << "AUTOINPUT_EXE_PATH is not available.";
        }

        autoinput::services::StdioProcessTransport transport(
            autoinputExePath(),
            std::vector<std::string>{ "serve", "--stdio" }
        );

        ASSERT_TRUE(transport.start()) << transport.lastError();

        ASSERT_TRUE(transport.writeLine("{\"id\":1,\"method\":\"stop\"}"));
        auto response = transport.readLine(std::chrono::milliseconds(5000));
        ASSERT_TRUE(response.has_value()) << "Timeout reading stop response";

        auto result = autoinput::services::parseRuntimeResponse(*response);
        EXPECT_TRUE(result.success);
        EXPECT_EQ(result.status, autoinput::services::RuntimeStatus::Stopped);
        EXPECT_EQ(result.message, "Automation stopped.");

        ASSERT_TRUE(transport.writeLine("{\"id\":2,\"method\":\"shutdown\"}"));
        response = transport.readLine(std::chrono::milliseconds(5000));
        ASSERT_TRUE(response.has_value());

        transport.stop();
    }

    TEST(ProcessTransportIntegrationTest, StdioTransportCanSendPause)
    {
        if (!canRunAutoinputExe())
        {
            GTEST_SKIP() << "AUTOINPUT_EXE_PATH is not available.";
        }

        autoinput::services::StdioProcessTransport transport(
            autoinputExePath(),
            std::vector<std::string>{ "serve", "--stdio" }
        );

        ASSERT_TRUE(transport.start()) << transport.lastError();

        ASSERT_TRUE(transport.writeLine("{\"id\":1,\"method\":\"pause\"}"));
        auto response = transport.readLine(std::chrono::milliseconds(5000));
        ASSERT_TRUE(response.has_value()) << "Timeout reading pause response";

        auto result = autoinput::services::parseRuntimeResponse(*response);
        // Pause is currently not implemented in serve mode or in-process runtime
        EXPECT_FALSE(result.success);
        EXPECT_EQ(result.status, autoinput::services::RuntimeStatus::Stopped);
        EXPECT_FALSE(result.message.empty());

        ASSERT_TRUE(transport.writeLine("{\"id\":2,\"method\":\"shutdown\"}"));
        response = transport.readLine(std::chrono::milliseconds(5000));
        ASSERT_TRUE(response.has_value());

        transport.stop();
    }

    TEST(ProcessRuntimeClientIntegrationTest, ProcessClientCanSendStop)
    {
        if (!canRunAutoinputExe())
        {
            GTEST_SKIP() << "AUTOINPUT_EXE_PATH is not available.";
        }

        auto transport = std::make_unique<autoinput::services::StdioProcessTransport>(
            autoinputExePath(),
            std::vector<std::string>{ "serve", "--stdio" }
        );

        autoinput::services::ProcessAutomationRuntimeClient client(std::move(transport));

        auto result = client.stop();

        EXPECT_TRUE(result.success);
        EXPECT_EQ(result.status, autoinput::services::RuntimeStatus::Stopped);
        EXPECT_EQ(result.message, "Automation stopped.");
    }
}
