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
        EXPECT_EQ(runtimeStatusToString(RuntimeStatus::Stopped), "stopped");
        EXPECT_EQ(runtimeStatusToString(RuntimeStatus::Starting), "starting");
        EXPECT_EQ(runtimeStatusToString(RuntimeStatus::Running), "running");
        EXPECT_EQ(runtimeStatusToString(RuntimeStatus::Paused), "paused");
        EXPECT_EQ(runtimeStatusToString(RuntimeStatus::Error), "error");

        EXPECT_EQ(runtimeStatusFromString("stopped"), RuntimeStatus::Stopped);
        EXPECT_EQ(runtimeStatusFromString("starting"), RuntimeStatus::Starting);
        EXPECT_EQ(runtimeStatusFromString("running"), RuntimeStatus::Running);
        EXPECT_EQ(runtimeStatusFromString("paused"), RuntimeStatus::Paused);
        EXPECT_EQ(runtimeStatusFromString("error"), RuntimeStatus::Error);
        EXPECT_EQ(runtimeStatusFromString("unknown"), RuntimeStatus::Stopped);
    }

    TEST(RuntimeProtocolTest, BuildRequests)
    {
        EXPECT_EQ(buildRuntimeRequest(1, "status"), "{\"id\":1,\"method\":\"status\"}");
        EXPECT_EQ(buildRuntimeRequest(2, "stop"), "{\"id\":2,\"method\":\"stop\"}");
        EXPECT_EQ(buildStartRuntimeRequest(3, "my-config"), "{\"id\":3,\"method\":\"start\",\"params\":{\"config\":\"my-config\"}}");

        EXPECT_EQ(
            buildStartRuntimeRequest(1, "my \"config\""),
            "{\"id\":1,\"method\":\"start\",\"params\":{\"config\":\"my \\\"config\\\"\"}}"
        );
    }

    TEST(RuntimeProtocolTest, BuildResponse)
    {
        RuntimeOperationResult result{ true, RuntimeStatus::Running, "Started successfully" };
        std::string json = buildRuntimeResponse(10, result);
        EXPECT_EQ(json, "{\"id\":10,\"success\":true,\"status\":\"running\",\"message\":\"Started successfully\"}");

        result = { false, RuntimeStatus::Stopped, "Failed to start" };
        json = buildRuntimeResponse(11, result);
        EXPECT_EQ(json, "{\"id\":11,\"success\":false,\"status\":\"stopped\",\"message\":\"Failed to start\"}");

        result = {
            false,
            RuntimeStatus::Error,
            "Bad \"config\"\nTry again"
        };

        EXPECT_EQ(
            buildRuntimeResponse(1, result),
            "{\"id\":1,\"success\":false,\"status\":\"error\",\"message\":\"Bad \\\"config\\\"\\nTry again\"}"
        );
    }

    TEST(RuntimeProtocolTest, ParseRequest)
    {
        auto req = parseRuntimeRequest("{\"id\":1,\"method\":\"status\"}");
        EXPECT_TRUE(req.valid);
        EXPECT_EQ(req.id, 1);
        EXPECT_EQ(req.method, "status");

        req = parseRuntimeRequest("{\"id\":2,\"method\":\"start\",\"params\":{\"config\":\"auto\"}}");
        EXPECT_TRUE(req.valid);
        EXPECT_EQ(req.id, 2);
        EXPECT_EQ(req.method, "start");
        EXPECT_EQ(req.config, "auto");

        req = parseRuntimeRequest("  {\"id\": 3, \"method\": \"pause\"}  ");
        EXPECT_TRUE(req.valid);
        EXPECT_EQ(req.id, 3);
        EXPECT_EQ(req.method, "pause");

        req = parseRuntimeRequest(
            "{\"id\":1,\"method\":\"start\",\"params\":{\"config\":\"my \\\"config\\\"\"}}"
        );
        EXPECT_TRUE(req.valid);
        EXPECT_EQ(req.config, "my \"config\"");
    }

    TEST(RuntimeProtocolTest, ParseInvalidRequest)
    {
        auto req = parseRuntimeRequest("invalid");
        EXPECT_FALSE(req.valid);

        req = parseRuntimeRequest("{\"id\":-1,\"method\":\"status\"}");
        EXPECT_FALSE(req.valid);

        req = parseRuntimeRequest("{\"id\":1abc,\"method\":\"status\"}");
        EXPECT_FALSE(req.valid);

        req = parseRuntimeRequest("{\"method\":\"status\"}");
        EXPECT_FALSE(req.valid);
        EXPECT_EQ(req.error, "Missing request ID.");

        req = parseRuntimeRequest("{\"id\":\"abc\",\"method\":\"status\"}");
        EXPECT_FALSE(req.valid);
        EXPECT_EQ(req.error, "Invalid request ID.");

        req = parseRuntimeRequest("{\"id\":1}"); // Missing method
        EXPECT_FALSE(req.valid);
        EXPECT_EQ(req.error, "Missing method.");

        req = parseRuntimeRequest("{\"id\":1,\"method\":\"start\"}");
        EXPECT_FALSE(req.valid);
        EXPECT_EQ(req.error, "Missing config for start request.");

        req = parseRuntimeRequest("{\"id\":1,\"method\":\"start\",\"params\":{\"config\":\"\"}}");
        EXPECT_FALSE(req.valid);
        EXPECT_EQ(req.error, "Missing config for start request.");

        req = parseRuntimeRequest("{\"id\":1,\"method\":123}");
        EXPECT_FALSE(req.valid);
        EXPECT_EQ(req.error, "Invalid method.");
    }

    TEST(RuntimeProtocolTest, ParseResponse)
    {
        auto res = parseRuntimeResponse("{\"id\":1,\"success\":true,\"status\":\"running\",\"message\":\"OK\"}");
        EXPECT_TRUE(res.success);
        EXPECT_EQ(res.status, RuntimeStatus::Running);
        EXPECT_EQ(res.message, "OK");

        res = parseRuntimeResponse(
            "{\"id\":1,\"success\":false,\"status\":\"error\",\"message\":\"Bad \\\"config\\\"\\nTry again\"}"
        );
        EXPECT_FALSE(res.success);
        EXPECT_EQ(res.status, RuntimeStatus::Error);
        EXPECT_EQ(res.message, "Bad \"config\"\nTry again");
    }
}
