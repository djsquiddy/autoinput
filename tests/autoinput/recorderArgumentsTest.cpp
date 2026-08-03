/**
 * @file recorderArgumentsTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/arguments.h"
#include <vector>
#include <string>

namespace autoinput
{
    TEST(RecorderArgumentsTest, ParseRecordingOptions)
    {
        ProgramArguments args;
        std::vector<char*> argv = {
            const_cast<char*>("autoinput"),
            const_cast<char*>("--record"), const_cast<char*>("my-macro"),
            const_cast<char*>("--record-start"), const_cast<char*>("f10"),
            const_cast<char*>("--record-end"), const_cast<char*>("f11"),
            const_cast<char*>("--record-play-start"), const_cast<char*>("f12"),
            const_cast<char*>("--record-mouse-moves"),
            const_cast<char*>("--record-mouse-sample"), const_cast<char*>("50ms"),
            const_cast<char*>("--force")
        };

        ASSERT_TRUE(args.parseArguments(argv));
        EXPECT_EQ(args.recordName, "my-macro");
        EXPECT_EQ(args.recordStartKey, "f10");
        EXPECT_EQ(args.recordEndKey, "f11");
        EXPECT_EQ(args.recordPlayStartKey, "f12");
        EXPECT_TRUE(args.recordMouseMoves);
        EXPECT_EQ(args.recordMouseSample, "50ms");
        EXPECT_TRUE(args.forceOverwrite);
        
        // Check resolved save path
        EXPECT_FALSE(args.saveConfigName.empty());
        EXPECT_TRUE(args.saveConfigName.find("my-macro.toml") != std::string::npos);
    }

    TEST(RecorderArgumentsTest, DefaultRecordingOptions)
    {
        ProgramArguments args;
        std::vector<char*> argv = {
            const_cast<char*>("autoinput"),
            const_cast<char*>("--record"), const_cast<char*>("my-macro")
        };

        ASSERT_TRUE(args.parseArguments(argv));
        EXPECT_EQ(args.recordName, "my-macro");
        EXPECT_EQ(args.recordStartKey, "f8");
        EXPECT_EQ(args.recordEndKey, "f9");
        EXPECT_EQ(args.recordPlayStartKey, "f6");
        EXPECT_FALSE(args.recordMouseMoves);
        EXPECT_EQ(args.recordMouseSample, "25ms");
    }
}
