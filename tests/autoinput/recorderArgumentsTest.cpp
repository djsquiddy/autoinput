/**
 * @file recorderArgumentsTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/cli/cliApplication.h"
#include <vector>
#include <string>
#include <gsl/gsl>

namespace autoinput::cli
{
    namespace
    {
        std::vector<char*> toArgv(const std::vector<std::string>& args)
        {
            std::vector<char*> argv;
            argv.reserve(args.size());
            for (const auto& arg : args)
            {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            return argv;
        }
    }

    TEST(RecorderArgumentsTest, ParseRecordingOptions)
    {
        CliApplication app;
        std::vector<std::string> args = {
            "autoinput",
            "record", "my-macro",
            "--start", "f10",
            "--end", "f11",
            "--play-start", "f12",
            "--mouse-moves",
            "--mouse-sample", "50ms",
            "--force"
        };
        auto argv = toArgv(args);

        ASSERT_TRUE(app.parse(gsl::make_span(argv)));
    }

    TEST(RecorderArgumentsTest, DefaultRecordingOptions)
    {
        CliApplication app;
        std::vector<std::string> args = {
            "autoinput",
            "record", "my-macro"
        };
        auto argv = toArgv(args);

        ASSERT_TRUE(app.parse(gsl::make_span(argv)));
    }

    TEST(RecorderArgumentsTest, RejectsOldSyntax)
    {
        CliApplication app;
        std::vector<std::string> args = {
            "autoinput",
            "--record", "my-macro"
        };
        auto argv = toArgv(args);

        ASSERT_FALSE(app.parse(gsl::make_span(argv)));
    }
}
