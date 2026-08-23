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

        // Ensure the stdio process transport starts successfully
        ASSERT_TRUE(transport.start()) << transport.lastError();

        // Ensure status request line is written to process transport without errors
        ASSERT_TRUE(transport.writeLine("{\"id\":1,\"method\":\"status\"}"));
        auto response = transport.readLine(std::chrono::milliseconds(5000));
        // Ensure status response is received from process before timeout
        ASSERT_TRUE(response.has_value()) << "Timeout reading status response";

        auto result = autoinput::services::parseRuntimeResponse(*response);
        // Verify the status request was processed successfully
        EXPECT_TRUE(result.success);
        // Verify the initial runtime status reported is Stopped
        EXPECT_EQ(result.status, autoinput::services::RuntimeStatus::Stopped);
        // Verify the response message matches the expected status message
        EXPECT_EQ(result.message, "Status retrieved.");

        // Ensure shutdown request line is written to process transport
        ASSERT_TRUE(transport.writeLine("{\"id\":2,\"method\":\"shutdown\"}"));
        response = transport.readLine(std::chrono::milliseconds(5000));
        // Ensure shutdown response is received before timeout
        ASSERT_TRUE(response.has_value()) << "Timeout reading shutdown response";

        result = autoinput::services::parseRuntimeResponse(*response);
        // Verify the shutdown request succeeded
        EXPECT_TRUE(result.success);
        // Verify the runtime remains in Stopped status upon shutdown
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

        // Ensure the stdio process transport starts successfully
        ASSERT_TRUE(transport.start()) << transport.lastError();

        // Ensure stop request line is written to process transport
        ASSERT_TRUE(transport.writeLine("{\"id\":1,\"method\":\"stop\"}"));
        auto response = transport.readLine(std::chrono::milliseconds(5000));
        // Ensure stop response is received before timeout
        ASSERT_TRUE(response.has_value()) << "Timeout reading stop response";

        auto result = autoinput::services::parseRuntimeResponse(*response);
        // Verify the stop request succeeded
        EXPECT_TRUE(result.success);
        // Verify runtime status is Stopped after stop command
        EXPECT_EQ(result.status, autoinput::services::RuntimeStatus::Stopped);
        // Verify the response message indicates automation was stopped
        EXPECT_EQ(result.message, "Automation stopped.");

        // Ensure shutdown request is sent to terminate the process cleanly
        ASSERT_TRUE(transport.writeLine("{\"id\":2,\"method\":\"shutdown\"}"));
        response = transport.readLine(std::chrono::milliseconds(5000));
        // Ensure shutdown response is received before timeout
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

        // Ensure the stdio process transport starts successfully
        ASSERT_TRUE(transport.start()) << transport.lastError();

        // Ensure pause request line is written to process transport
        ASSERT_TRUE(transport.writeLine("{\"id\":1,\"method\":\"pause\"}"));
        auto response = transport.readLine(std::chrono::milliseconds(5000));
        // Ensure pause response is received before timeout
        ASSERT_TRUE(response.has_value()) << "Timeout reading pause response";

        auto result = autoinput::services::parseRuntimeResponse(*response);
        // Pause is currently not implemented in serve mode or in-process runtime
        // Verify pause is reported as unsuccessful because pause is not implemented in serve mode
        EXPECT_FALSE(result.success);
        // Verify runtime status remains Stopped
        EXPECT_EQ(result.status, autoinput::services::RuntimeStatus::Stopped);
        // Verify an explanatory error message is returned
        EXPECT_FALSE(result.message.empty());

        // Ensure shutdown request is written to clean up process
        ASSERT_TRUE(transport.writeLine("{\"id\":2,\"method\":\"shutdown\"}"));
        response = transport.readLine(std::chrono::milliseconds(5000));
        // Ensure shutdown response is received before timeout
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

        // Verify the client stop method succeeded
        EXPECT_TRUE(result.success);
        // Verify runtime status returned by client is Stopped
        EXPECT_EQ(result.status, autoinput::services::RuntimeStatus::Stopped);
        // Verify message returned by client indicates automation stopped
        EXPECT_EQ(result.message, "Automation stopped.");
    }
}
