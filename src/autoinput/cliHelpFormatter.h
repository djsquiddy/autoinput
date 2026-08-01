/**
* @file cliHelpFormatter.h
* @author djsquiddy
* @date August 2026
*/
#ifndef INCLUDE_AUTOINPUT_CLI_HELP_FORMATTER_H
#define INCLUDE_AUTOINPUT_CLI_HELP_FORMATTER_H
#pragma once

#include <string_view>

namespace autoinput
{
    class CliHelpFormatter
    {
    public:
        static void printUsage(std::string_view programName, bool verbose = false);
    };
}

#endif // INCLUDE_AUTOINPUT_CLI_HELP_FORMATTER_H
