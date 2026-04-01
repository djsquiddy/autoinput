
/*
 *
 * TODO: need to separate the code to be platform independent.
*/
#include <iostream>

#include "autoinput.h"

int main(int argc, char* argv[])
{
    using namespace autoinput;
    g_program = std::make_unique<Program>();
    if (!g_program->arguments().parseArguments(argc, argv))
    {
        return EXIT_FAILURE;
    }

    g_program->init();
    g_program->printProgramInfo();
    std::cout << "Global keyboard listener started. Press Ctrl+C to exit.\n\n";

    if (!installHooks())
    {
        return EXIT_FAILURE;
    }
    runListener();
    cleanup();

    return EXIT_SUCCESS;
}