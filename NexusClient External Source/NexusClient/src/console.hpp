
#include <windows.h>
#include <iostream>
#include <cstdio>
#include <random>
#include <string>
#include <thread>
#include <atomic>

namespace Logger
{
    inline std::atomic<bool> running = true;

    inline std::string RandomTitle(std::size_t length = 32)
    {
        static constexpr char chars[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789";

        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dist(0, sizeof(chars) - 2);
        std::string result;
        result.reserve(length);
        for (std::size_t i = 0; i < length; ++i)
            result += chars[dist(gen)];
        return result;
    }

    inline void Initialize(const char* consoleTitle)
    {
        AllocConsole();
        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONIN$", "r", stdin);
        SetConsoleTitleA(consoleTitle);
        std::thread([]()
            {
                while (running)
                {
                    SetConsoleTitleA(RandomTitle().c_str());
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }).detach();
    }
    inline void Shutdown()
    {
        running = false;
        std::cout << "[+] exiting...\n";
        FreeConsole();
    }
}
