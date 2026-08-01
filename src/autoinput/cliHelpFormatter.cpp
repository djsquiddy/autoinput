/**
* @file cliHelpFormatter.cpp
* @author djsquiddy
* @date August 2026
*/
#include "autoinput/cliHelpFormatter.h"
#include "autoinput/defaults.h"
#include "autoinput/logger.h"
#include "autoinput/config.h"
#include "autoinput/types.h"

namespace autoinput
{
    void CliHelpFormatter::printUsage(const std::string_view programName, const bool verbose)
    {
        Logger::print("usage {} [-h] [{{click,hold}}] [{{left,right,middle,key}} ...] [-s START_KEYS [START_KEYS ...]] [-e END_KEY] [-w WAIT_TIME] [-S SAVE_CONFIG_NAME]\n\n", programName);
        Logger::print("options\n");
        const auto optionPrefix = std::string(4, ' ');
        const auto optionUsagePrefix = std::string(10, ' ');

        Logger::print("{} -h, --help\n", optionPrefix);
        Logger::print("{} show this help message with examples and exits.\n", optionUsagePrefix);
        Logger::print("{} -l, --log [{{d,debug,i,info,w,warn,warning,e,error,f,fatal}}]\n", optionPrefix);
        Logger::print("{} set the log level. (Choices: debug, info, warning, warn, error, fatal)\n", optionUsagePrefix);
        Logger::print("{} -c --config\n", optionPrefix);
        Logger::print("{} Use the specified configuration found under {}. Extension can be omitted\n", optionUsagePrefix, getConfigsPath().string());
        Logger::print("{} -t, --type {{click,c,hold,h}}\n", optionPrefix);
        Logger::print("{} What kind of action event to use. (Can be positional)\n", optionUsagePrefix);
#if AUTOINPUT_HOOK_MOUSE_ENABLED
        Logger::print("{} -b, --btn, --button {{button}} [{{button}} ...]\n", optionPrefix);
        Logger::print("{} Which button to press. (Default: {}) (Can be positional). Modifiers like shift+left are supported.\n", optionUsagePrefix, defaults::DefaultMouseButtonName);
#endif // AUTOINPUT_HOOK_MOUSE_ENABLED
#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
        Logger::print("{} -k, --key {{key}} [{{key}} ...]\n", optionPrefix);
        Logger::print("{} Key that is to be pressed/simulated. (Can be positional)\n", optionUsagePrefix);
#endif
        Logger::print("{} -s, --start-key START_KEYS [START_KEYS ...]\n", optionPrefix);
        Logger::print("{} Key that is used to start the autoclicker. If button presses need separate start/stop binding the order matters here.\n", optionUsagePrefix);
        Logger::print("{} -e, --end-key END_KEY\n", optionPrefix);
        Logger::print("{} Key that is used to end the autoclicker.\n", optionUsagePrefix);
        Logger::print("{} -a, --app, --application APPLICATION_NAME\n", optionPrefix);
        Logger::print("{} Only listen for inputs when this application is in focus.\n", optionUsagePrefix);
        Logger::print("{} -B, --blacklist APPLICATION_NAME\n", optionPrefix);
        Logger::print("{} Do not run when this application is in focus.\n", optionUsagePrefix);
        Logger::print("{} -L, --list-apps\n", optionPrefix);
        Logger::print("{} List all currently running application names and exit.\n", optionUsagePrefix);
        Logger::print("{} -C, --list-configs\n", optionPrefix);
        Logger::print("{} List all valid configuration names and exit.\n", optionUsagePrefix);
        Logger::print("{} -S, --save-config SAVE_CONFIG_NAME\n", optionPrefix);
        Logger::print("{} Save the current configuration into a named configuration in the user configurations and exit.\n", optionUsagePrefix);
        Logger::print("{} -w, --wait WAIT_TIME\n", optionPrefix);
        Logger::print("{} Max duration to wait before auto clicking. {}\n", optionUsagePrefix, bold("Type must be set to click"));
        Logger::print("{} Can use a time range by using {{min}}..{{max}} with each click being random between the range. See examples for usage.\n", optionUsagePrefix);

        Logger::print("\n\n");
        if (verbose)
        {
            const auto examplePrefix = std::string(2, ' ');
            const auto exampleCmdPrefix = std::string(4, ' ') + "> ";
            const auto exampleNotePrefix = std::string(10, ' ') + "Note: ";
            int32_t index = 1;
            Logger::print("examples:\n");
            Logger::print("{}{}) hold the {} click when pressing {} and stopping on {} (Defaults):\n", examplePrefix, index, defaults::DefaultMouseButtonName, defaults::StartKey, defaults::EndKey);
            Logger::print("{}{} hold {}\n", exampleCmdPrefix, programName, defaults::DefaultMouseButtonName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the left click when pressing F2, or hold the right click when pressing F3 and stop on F4:\n", examplePrefix, index);
            Logger::print("{}{} hold left f2 right f3 -e f4\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) click the left button when pressing F2 and hold the left button when pressing F4:\n", examplePrefix, index);
            Logger::print("{}{} click f2 hold f4\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the left button when pressing the mouse BACK button:\n", examplePrefix, index);
            Logger::print("{}{} hold left back\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the left and right click when pressing F2 and stop on F3:\n", examplePrefix, index);
            Logger::print("{}{} hold left right f2\n", exampleCmdPrefix, programName);
            Logger::print("{} This only works if start key only has {} value.\n", exampleNotePrefix, bold("ONE"));
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) auto left click every 2 seconds:\n", examplePrefix, index);
            Logger::print("{}{} left -w 2s\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) auto left click every 1 to 2 seconds:\n", examplePrefix, index);
            Logger::print("{}{} left --press-wait 1s..2s\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) auto left click and hold for 500ms to 1s then wait 1s to 2s after to click again:\n", examplePrefix, index);
            Logger::print("{}{} left --press-wait 500ms..1s --release-wait 1s..2s\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) simulate pressing the 'a' key when F2 is pressed:\n", examplePrefix, index);
            Logger::print("{}{} a f2\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the space bar when pressing F2 and stop on F3:\n", examplePrefix, index);
            Logger::print("{}{} space f2 -e f3\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) click the 'ctrl+v' key combination when pressing F2:\n", examplePrefix, index);
            Logger::print("{}{} click ctrl+v f2\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) click the 'shift+left' mouse button when pressing F2:\n", examplePrefix, index);
            Logger::print("{}{} click shift+left f2\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) use a configuration file named 'gaming':\n", examplePrefix, index);
            Logger::print("{}{} --config gaming\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) run with debug logging enabled:\n", examplePrefix, index);
            Logger::print("{}{} -l debug hold left\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the left click but stop running if notepad is in focus:\n", examplePrefix, index);
            Logger::print("{}{} hold left --blacklist notepad.exe\n", exampleCmdPrefix, programName);
        }

        Logger::flush();
    }
}
