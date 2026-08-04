/**
 * @file positionalArgumentsTest.cpp
 * @author djsquiddy
 * @date July 2026
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

    TEST(PositionalArgumentsTest, RejectsLegacyPositionalSyntax)
    {
        const std::vector<std::vector<std::string>> cases = {
            {"autoinput", "click", "left"},
            {"autoinput", "left"},
            {"autoinput", "left", "right"},
            {"autoinput", "hold", "left", "right"},
            {"autoinput", "a"},
            {"autoinput", "left", "-s", "back", "-e", "forward"},
            {"autoinput", "left", "f4", "right", "f5"}
        };

        for (const auto& args : cases)
        {
            CliApplication app;
            auto argv = toArgv(args);
            EXPECT_FALSE(app.parse(gsl::make_span(argv))) << "Should have rejected legacy syntax: " << args[1];
        }
    }

    TEST(PositionalArgumentsTest, ParsesNewEquivalentSyntax)
    {
        CliApplication app;
        std::vector<std::string> args = {"autoinput", "run", "--type", "hold", "--button", "left"};
        auto argv = toArgv(args);
        EXPECT_TRUE(app.parse(gsl::make_span(argv)));
    }
}
