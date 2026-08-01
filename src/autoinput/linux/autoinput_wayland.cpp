/**
 * @file autoInput_wayland.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/autoInput.h"
#include "autoinput/mouse.h"
#include "autoinput/keyboard.h"
#include "autoinput/logger.h"
#include "autoinput/types.h"
#include "autoinput/linux/internalData_linux.h"

namespace autoinput
{
    namespace
    {
        int g_uinputFd = -1;
        bool g_running = true;
        std::vector<int> g_evdevFds;

        void setupUinput()
        {
            g_uinputFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
            if (g_uinputFd < 0) return;

            ioctl(g_uinputFd, UI_SET_EVBIT, EV_KEY);
            ioctl(g_uinputFd, UI_SET_EVBIT, EV_SYN);

            // Keys
            for (int i = 0; i < KEY_MAX; ++i) ioctl(g_uinputFd, UI_SET_KEYBIT, i);
            
            // Buttons
            for (int i = BTN_MOUSE; i < BTN_JOYSTICK; ++i) ioctl(g_uinputFd, UI_SET_KEYBIT, i);

            struct uinput_setup us;
            memset(&us, 0, sizeof(us));
            us.id.bustype = BUS_USB;
            us.id.vendor = 0x1234;
            us.id.product = 0x5678;
            strcpy(us.name, "autoinput virtual device");

            ioctl(g_uinputFd, UI_DEV_SETUP, &us);
            ioctl(g_uinputFd, UI_DEV_CREATE);
        }

        uint16_t toEvdevKey(const Key& key)
        {
            if (static_cast<bool>(key.modifier & KeyModifier::Function))
            {
                const int functionNumber = std::stoi(key.character);
                if (functionNumber >= 1 && functionNumber <= 12) return KEY_F1 + functionNumber - 1;
                if (functionNumber >= 13 && functionNumber <= 24) return KEY_F13 + functionNumber - 13;
            }
            
            if (key.character.length() == 1)
            {
                char c = std::tolower(key.character[0]);
                if (c >= 'a' && c <= 'z') return KEY_A + (c - 'a');
                if (c >= '0' && c <= '9') return (c == '0') ? KEY_0 : KEY_1 + (c - '1');
                // Add more mappings as needed
            }
            return 0;
        }

        uint16_t toEvdevButton(MouseButton button)
        {
            switch (button)
            {
            case MouseButton::LEFT: return BTN_LEFT;
            case MouseButton::RIGHT: return BTN_RIGHT;
            case MouseButton::MIDDLE: return BTN_MIDDLE;
            case MouseButton::BACK: return BTN_SIDE;
            case MouseButton::FORWARD: return BTN_EXTRA;
            default: return 0;
            }
        }

        void emit(int fd, uint16_t type, uint16_t code, int val)
        {
            struct input_event ie;
            ie.type = type;
            ie.code = code;
            ie.value = val;
            ie.time.tv_sec = 0;
            ie.time.tv_usec = 0;
            write(fd, &ie, sizeof(ie));
        }

        class WaylandBackend : public IPlatformBackend
        {
        public:
            bool installHooks() override
            {
                setupUinput();
                
                std::error_code ec;
                if (!std::filesystem::exists("/dev/input", ec) || !std::filesystem::is_directory("/dev/input", ec))
                {
                    Logger::error("Failed to access /dev/input\n");
                    return false;
                }

                // Open all input devices for listening
                for (const auto& entry : std::filesystem::directory_iterator("/dev/input", ec))
                {
                    if (ec) break;
                    if (entry.path().filename().string().starts_with("event"))
                    {
                        int fd = open(entry.path().c_str(), O_RDONLY | O_NONBLOCK);
                        if (fd >= 0) g_evdevFds.push_back(fd);
                    }
                }
                
                if (g_evdevFds.empty())
                {
                    Logger::error("No input devices found in /dev/input\n");
                    return false;
                }

                if (g_program)
                {
                    g_program->onFocusChanged(getWaylandActiveApplicationName());
                }

                return true;
            }

            void runListener() override
            {
                std::vector<struct pollfd> pds;
                for (int fd : g_evdevFds) pds.push_back({fd, POLLIN, 0});

                while (g_running)
                {
                    int ret = poll(pds.data(), pds.size(), 100);
                    if (ret > 0)
                    {
                        for (auto& pd : pds)
                        {
                            if (pd.revents & POLLIN)
                            {
                                struct input_event ie;
                                while (read(pd.fd, &ie, sizeof(ie)) > 0)
                                {
                                    if (ie.type == EV_KEY)
                                    {
                                        if (ie.code >= BTN_MOUSE && ie.code < BTN_JOYSTICK)
                                        {
                                            WaylandMouseData wData{ ie.code, ie.value };
                                            MouseData mData{ wData };
                                            g_program->processMouseEvent(MouseInput{ mData });
                                        }
                                        else
                                        {
                                            WaylandKeyboardData wData{ ie.code, ie.value };
                                            KeyboardData kData{ wData };
                                            g_program->processKeyEvent(KeyboardInput{ kData });
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            void cleanup() override
            {
                if (g_uinputFd >= 0)
                {
                    ioctl(g_uinputFd, UI_DEV_DESTROY);
                    close(g_uinputFd);
                    g_uinputFd = -1;
                }
                for (int fd : g_evdevFds) close(fd);
                g_evdevFds.clear();
            }

            void keyPress(const Key& key) override
            {
                if (g_uinputFd < 0) return;
                uint16_t code = toEvdevKey(key);
                if (code == 0) return;
                
                if (static_cast<bool>(key.modifier & KeyModifier::Shift)) emit(g_uinputFd, EV_KEY, KEY_LEFTSHIFT, 1);
                if (static_cast<bool>(key.modifier & KeyModifier::Ctrl)) emit(g_uinputFd, EV_KEY, KEY_LEFTCTRL, 1);
                if (static_cast<bool>(key.modifier & KeyModifier::Alt)) emit(g_uinputFd, EV_KEY, KEY_LEFTALT, 1);
                if (static_cast<bool>(key.modifier & KeyModifier::Meta)) emit(g_uinputFd, EV_KEY, KEY_LEFTMETA, 1);
                
                emit(g_uinputFd, EV_KEY, code, 1);
                emit(g_uinputFd, EV_SYN, SYN_REPORT, 0);
            }

            void keyRelease(const Key& key) override
            {
                if (g_uinputFd < 0) return;
                uint16_t code = toEvdevKey(key);
                if (code == 0) return;
                
                emit(g_uinputFd, EV_KEY, code, 0);
                
                if (static_cast<bool>(key.modifier & KeyModifier::Meta)) emit(g_uinputFd, EV_KEY, KEY_LEFTMETA, 0);
                if (static_cast<bool>(key.modifier & KeyModifier::Alt)) emit(g_uinputFd, EV_KEY, KEY_LEFTALT, 0);
                if (static_cast<bool>(key.modifier & KeyModifier::Ctrl)) emit(g_uinputFd, EV_KEY, KEY_LEFTCTRL, 0);
                if (static_cast<bool>(key.modifier & KeyModifier::Shift)) emit(g_uinputFd, EV_KEY, KEY_LEFTSHIFT, 0);
                
                emit(g_uinputFd, EV_SYN, SYN_REPORT, 0);
            }

            void mousePress(const Mouse& mouse) override
            {
                if (g_uinputFd < 0) return;
                uint16_t code = toEvdevButton(mouse.button);
                if (code == 0) return;

                if (static_cast<bool>(mouse.modifier & KeyModifier::Shift)) emit(g_uinputFd, EV_KEY, KEY_LEFTSHIFT, 1);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Ctrl)) emit(g_uinputFd, EV_KEY, KEY_LEFTCTRL, 1);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Alt)) emit(g_uinputFd, EV_KEY, KEY_LEFTALT, 1);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Meta)) emit(g_uinputFd, EV_KEY, KEY_LEFTMETA, 1);

                emit(g_uinputFd, EV_KEY, code, 1);
                emit(g_uinputFd, EV_SYN, SYN_REPORT, 0);
            }

            void mouseRelease(const Mouse& mouse) override
            {
                if (g_uinputFd < 0) return;
                uint16_t code = toEvdevButton(mouse.button);
                if (code == 0) return;

                emit(g_uinputFd, EV_KEY, code, 0);

                if (static_cast<bool>(mouse.modifier & KeyModifier::Meta)) emit(g_uinputFd, EV_KEY, KEY_LEFTMETA, 0);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Alt)) emit(g_uinputFd, EV_KEY, KEY_LEFTALT, 0);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Ctrl)) emit(g_uinputFd, EV_KEY, KEY_LEFTCTRL, 0);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Shift)) emit(g_uinputFd, EV_KEY, KEY_LEFTSHIFT, 0);

                emit(g_uinputFd, EV_SYN, SYN_REPORT, 0);
            }
        };
    }

    int32_t getWaylandVirtualKey(const WaylandKeyboardData& data)
    {
        return static_cast<int32_t>(data.code);
    }

    std::string getWaylandActiveApplicationName()
    {
        // Wayland does not allow getting the focused window name for security reasons
        // unless using a compositor-specific protocol or a portal.
        return "";
    }

    std::vector<std::string> getWaylandRunningApplicationNames()
    {
        return {};
    }

    std::unique_ptr<IPlatformBackend> createWaylandBackend()
    {
        return std::make_unique<WaylandBackend>();
    }

    bool isWaylandKeyDown(const WaylandKeyboardData& data) { return data.value == 1 || data.value == 2; }
    bool isWaylandKeyUp(const WaylandKeyboardData& data) { return data.value == 0; }
    int8_t getWaylandChar(const WaylandKeyboardData& data)
    {
        if (data.code >= KEY_A && data.code <= KEY_Z) return 'a' + (data.code - KEY_A);
        return INVALID_KEY;
    }
    int64_t getWaylandFunctionKey(const WaylandKeyboardData& data)
    {
        if (data.code >= KEY_F1 && data.code <= KEY_F12) return data.code - KEY_F1 + 1;
        if (data.code >= KEY_F13 && data.code <= KEY_F24) return data.code - KEY_F13 + 13;
        return INVALID_KEY;
    }
    void printWaylandKeyboardInfo(const WaylandKeyboardData& data)
    {
        if (!Logger::isDebugModeEnabled()) return;
        Logger::debug("[Wayland] keycode={} value={}\n", data.code, data.value);
    }

    bool isWaylandLeftButtonDown(const WaylandMouseData& data) { return data.code == BTN_LEFT && data.value == 1; }
    bool isWaylandLeftButtonUp(const WaylandMouseData& data) { return data.code == BTN_LEFT && data.value == 0; }
    bool isWaylandRightButtonDown(const WaylandMouseData& data) { return data.code == BTN_RIGHT && data.value == 1; }
    bool isWaylandRightButtonUp(const WaylandMouseData& data) { return data.code == BTN_RIGHT && data.value == 0; }
    bool isWaylandMiddleButtonDown(const WaylandMouseData& data) { return data.code == BTN_MIDDLE && data.value == 1; }
    bool isWaylandMiddleButtonUp(const WaylandMouseData& data) { return data.code == BTN_MIDDLE && data.value == 0; }
    bool isWaylandBackButtonDown(const WaylandMouseData& data) { return data.code == BTN_SIDE && data.value == 1; }
    bool isWaylandBackButtonUp(const WaylandMouseData& data) { return data.code == BTN_SIDE && data.value == 0; }
    bool isWaylandForwardButtonDown(const WaylandMouseData& data) { return data.code == BTN_EXTRA && data.value == 1; }
    bool isWaylandForwardButtonUp(const WaylandMouseData& data) { return data.code == BTN_EXTRA && data.value == 0; }
    void printWaylandMouseInfo(const WaylandMouseData& data)
    {
        if (!Logger::isDebugModeEnabled()) return;
        Logger::debug("[Wayland] button={} value={}\n", data.code, data.value);
    }
}
