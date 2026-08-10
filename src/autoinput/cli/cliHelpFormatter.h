/**
 * @file cliHelpFormatter.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_CLI_CLIHELPFORMATTER_H
#define INCLUDE_AUTOINPUT_CLI_CLIHELPFORMATTER_H
#pragma once

#include <string_view>

namespace autoinput
{
    class CliHelpFormatter
    {
    public:
        /**
         * @brief Prints the command-line usage information.
         * @param programName The name of the program to display in the usage.
         * @param verbose Whether to show detailed information.
         */
        static void printUsage(std::string_view programName, bool verbose = false);
    };
}

#endif // INCLUDE_AUTOINPUT_CLI_CLIHELPFORMATTER_H
