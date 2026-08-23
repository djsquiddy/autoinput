/**
 * @file runtimeProtocolTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/services/runtimeProtocol.h"

namespace autoinput::services
{
    TEST(RuntimeProtocolTest, StatusStringConversion)
    {
        // Verify conversion of RuntimeStatus::Stopped to string representation
        EXPECT_EQ(runtimeStatusToString(RuntimeStatus::Stopped), "stopped");
        // Verify conversion of RuntimeStatus::Starting to string representation
        EXPECT_EQ(runtimeStatusToString(RuntimeStatus::Starting), "starting");
        // Verify conversion of RuntimeStatus::Running to string representation
        EXPECT_EQ(runtimeStatusToString(RuntimeStatus::Running), "running");
        // Verify conversion of RuntimeStatus::Paused to string representation
        EXPECT_EQ(runtimeStatusToString(RuntimeStatus::Paused), "paused");
        // Verify conversion of RuntimeStatus::Error to string representation
        EXPECT_EQ(runtimeStatusToString(RuntimeStatus::Error), "error");

        // Verify parsing 'stopped' string into RuntimeStatus::Stopped enum value
        EXPECT_EQ(runtimeStatusFromString("stopped"), RuntimeStatus::Stopped);
        // Verify parsing 'starting' string into RuntimeStatus::Starting enum value
        EXPECT_EQ(runtimeStatusFromString("starting"), RuntimeStatus::Starting);
        // Verify parsing 'running' string into RuntimeStatus::Running enum value
        EXPECT_EQ(runtimeStatusFromString("running"), RuntimeStatus::Running);
        // Verify parsing 'paused' string into RuntimeStatus::Paused enum value
        EXPECT_EQ(runtimeStatusFromString("paused"), RuntimeStatus::Paused);
        // Verify parsing 'error' string into RuntimeStatus::Error enum value
        EXPECT_EQ(runtimeStatusFromString("error"), RuntimeStatus::Error);
        // Verify unknown status string defaults to RuntimeStatus::Stopped
        EXPECT_EQ(runtimeStatusFromString("unknown"), RuntimeStatus::Stopped);
    }

    TEST(RuntimeProtocolTest, BuildRequests)
    {
        // Verify JSON payload serialization for a status request
        EXPECT_EQ(buildRuntimeRequest(1, "status"), "{\"id\":1,\"method\":\"status\"}");
        // Verify JSON payload serialization for a stop request
        EXPECT_EQ(buildRuntimeRequest(2, "stop"), "{\"id\":2,\"method\":\"stop\"}");
        // Verify JSON payload serialization for a start request with config parameter
        EXPECT_EQ(buildStartRuntimeRequest(3, "my-config"), "{\"id\":3,\"method\":\"start\",\"params\":{\"config\":\"my-config\"}}");

        // Verify quotes in config name parameter are properly escaped in JSON request
        EXPECT_EQ(
            buildStartRuntimeRequest(1, "my \"config\""),
            "{\"id\":1,\"method\":\"start\",\"params\":{\"config\":\"my \\\"config\\\"\"}}"
        );

        // Verify JSON payload serialization for a run_command request with config and command parameters
        EXPECT_EQ(
            buildRunCommandRequest(4, "my-config", "my-command"),
            "{\"id\":4,\"method\":\"run_command\",\"params\":{\"config\":\"my-config\",\"command\":\"my-command\"}}"
        );
    }

    TEST(RuntimeProtocolTest, BuildResponse)
    {
        RuntimeOperationResult result{ true, RuntimeStatus::Running, "Started successfully" };
        std::string json = buildRuntimeResponse(10, result);
        // Verify JSON serialization of a successful running response
        EXPECT_EQ(json, "{\"id\":10,\"success\":true,\"status\":\"running\",\"message\":\"Started successfully\",\"recording\":false,\"recording_paused\":false,\"recorded_event_count\":0}");

        result = { false, RuntimeStatus::Stopped, "Failed to start" };
        json = buildRuntimeResponse(11, result);
        // Verify JSON serialization of a failed stopped response
        EXPECT_EQ(json, "{\"id\":11,\"success\":false,\"status\":\"stopped\",\"message\":\"Failed to start\",\"recording\":false,\"recording_paused\":false,\"recorded_event_count\":0}");

        result = {
            false,
            RuntimeStatus::Error,
            "Bad \"config\"\nTry again"
        };

        // Verify special characters and newlines in error response message are properly escaped in JSON
        EXPECT_EQ(
            buildRuntimeResponse(1, result),
            "{\"id\":1,\"success\":false,\"status\":\"error\",\"message\":\"Bad \\\"config\\\"\\nTry again\",\"recording\":false,\"recording_paused\":false,\"recorded_event_count\":0}"
        );
    }

    TEST(RuntimeProtocolTest, ParseRequest)
    {
        auto req = parseRuntimeRequest("{\"id\":1,\"method\":\"status\"}");
        // Verify valid status request string is parsed successfully
        EXPECT_TRUE(req.valid);
        // Verify request ID is parsed as 1
        EXPECT_EQ(req.id, 1);
        // Verify request method is parsed as 'status'
        EXPECT_EQ(req.method, "status");

        req = parseRuntimeRequest("{\"id\":2,\"method\":\"start\",\"params\":{\"config\":\"auto\"}}");
        // Verify valid start request string is parsed successfully
        EXPECT_TRUE(req.valid);
        // Verify request ID is parsed as 2
        EXPECT_EQ(req.id, 2);
        // Verify request method is parsed as 'start'
        EXPECT_EQ(req.method, "start");
        // Verify config parameter is parsed as 'auto'
        EXPECT_EQ(req.config, "auto");

        req = parseRuntimeRequest("  {\"id\": 3, \"method\": \"pause\"}  ");
        // Verify request with surrounding whitespace is parsed successfully
        EXPECT_TRUE(req.valid);
        // Verify request ID is parsed as 3
        EXPECT_EQ(req.id, 3);
        // Verify request method is parsed as 'pause'
        EXPECT_EQ(req.method, "pause");

        req = parseRuntimeRequest(
            "{\"id\":1,\"method\":\"start\",\"params\":{\"config\":\"my \\\"config\\\"\"}}"
        );
        // Verify start request containing escaped quotes in config parameter is valid
        EXPECT_TRUE(req.valid);
        // Verify unescaped config parameter value matches original string
        EXPECT_EQ(req.config, "my \"config\"");

        req = parseRuntimeRequest("{\"id\":4,\"method\":\"run_command\",\"params\":{\"config\":\"c\",\"command\":\"cmd\"}}");
        // Verify valid run_command request is parsed successfully
        EXPECT_TRUE(req.valid);
        // Verify method is parsed as 'run_command'
        EXPECT_EQ(req.method, "run_command");
        // Verify config parameter is parsed as 'c'
        EXPECT_EQ(req.config, "c");
        // Verify command parameter is parsed as 'cmd'
        EXPECT_EQ(req.command, "cmd");
    }

    TEST(RuntimeProtocolTest, ParseInvalidRequest)
    {
        auto req = parseRuntimeRequest("invalid");
        // Verify non-JSON string is marked as invalid
        EXPECT_FALSE(req.valid);

        req = parseRuntimeRequest("{\"id\":-1,\"method\":\"status\"}");
        // Verify request with negative ID is marked as invalid
        EXPECT_FALSE(req.valid);

        req = parseRuntimeRequest("{\"id\":1abc,\"method\":\"status\"}");
        // Verify request with non-numeric ID is marked as invalid
        EXPECT_FALSE(req.valid);

        req = parseRuntimeRequest("{\"method\":\"status\"}");
        // Verify request missing 'id' field is marked as invalid
        EXPECT_FALSE(req.valid);
        // Verify error message indicates missing request ID
        EXPECT_EQ(req.error, "Missing request ID.");

        req = parseRuntimeRequest("{\"id\":\"abc\",\"method\":\"status\"}");
        // Verify request with string ID is marked as invalid
        EXPECT_FALSE(req.valid);
        // Verify error message indicates invalid request ID
        EXPECT_EQ(req.error, "Invalid request ID.");

        req = parseRuntimeRequest("{\"id\":1}"); // Missing method
        // Verify request missing 'method' field is marked as invalid
        EXPECT_FALSE(req.valid);
        // Verify error message indicates missing method
        EXPECT_EQ(req.error, "Missing method.");

        req = parseRuntimeRequest("{\"id\":1,\"method\":\"start\"}");
        // Verify start request missing params object is marked as invalid
        EXPECT_FALSE(req.valid);
        // Verify error message indicates missing config for start request
        EXPECT_EQ(req.error, "Missing config for start request.");

        req = parseRuntimeRequest("{\"id\":1,\"method\":\"start\",\"params\":{\"config\":\"\"}}");
        // Verify start request with empty config string is marked as invalid
        EXPECT_FALSE(req.valid);
        // Verify error message indicates missing config for start request
        EXPECT_EQ(req.error, "Missing config for start request.");

        req = parseRuntimeRequest("{\"id\":1,\"method\":\"run_command\",\"params\":{\"config\":\"c\"}}");
        // Verify run_command request missing command parameter is marked as invalid
        EXPECT_FALSE(req.valid);
        // Verify error message indicates missing command for run_command request
        EXPECT_EQ(req.error, "Missing command for run_command request.");

        req = parseRuntimeRequest("{\"id\":1,\"method\":123}");
        // Verify request with non-string method type is marked as invalid
        EXPECT_FALSE(req.valid);
        // Verify error message indicates invalid method type
        EXPECT_EQ(req.error, "Invalid method.");
    }

    TEST(RuntimeProtocolTest, ParseResponse)
    {
        auto res = parseRuntimeResponse("{\"id\":1,\"success\":true,\"status\":\"running\",\"message\":\"OK\"}");
        // Verify success flag is parsed as true
        EXPECT_TRUE(res.success);
        // Verify status string 'running' is parsed into RuntimeStatus::Running
        EXPECT_EQ(res.status, RuntimeStatus::Running);
        // Verify message string 'OK' is parsed correctly
        EXPECT_EQ(res.message, "OK");

        res = parseRuntimeResponse(
            "{\"id\":1,\"success\":false,\"status\":\"error\",\"message\":\"Bad \\\"config\\\"\\nTry again\"}"
        );
        // Verify success flag is parsed as false
        EXPECT_FALSE(res.success);
        // Verify status string 'error' is parsed into RuntimeStatus::Error
        EXPECT_EQ(res.status, RuntimeStatus::Error);
        // Verify escaped characters and newlines in response message are properly unescaped
        EXPECT_EQ(res.message, "Bad \"config\"\nTry again");
    }
}
