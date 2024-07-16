#include "./window/window.hpp"
#include "./core/cheat.h"
#include "./core/a/a.h"
void initialize_cheat() {
    // Initialize the cheat functionalities, wait for the process and driver, and then start the cheat
    cheatCore cheatInstance;
    std::thread cheatThread(&cheatCore::cheat, &cheatInstance);

     cheatCore triggerInstance;
     std::thread triggerThread(&cheatCore::Trigger_bot, &triggerInstance);


     // Hide console window since we're making our own window, or use WinMain() instead.
   // ShowWindow(GetConsoleWindow(), SW_HIDE);

    overlay.shouldRun = true;
    overlay.RenderMenu = false;

    overlay.CreateOverlay();
    overlay.CreateDevice();
    overlay.CreateImGui();

    std::wcout << L"[>>] Hit insert to show the menu in this overlay!" << std::endl;
    overlay.SetForeground(GetConsoleWindow());

    while (overlay.shouldRun) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (GetAsyncKeyState(VK_END) || GetAsyncKeyState(VK_OEM_MINUS) & 1) {
            exit(1);
        }

        overlay.StartRender();

        if (overlay.RenderMenu) {
            overlay.Render();
        }
        else {
            if (var::watermark) {
                Misc::watermark();
            }
            if (var::crosshair1) {
                Misc::Crosshair();
            }
            if (var::drawEsp) {
                cheat::esp();
            }

        }

        overlay.EndRender();
    }

    overlay.DestroyImGui();
    overlay.DestroyDevice();
    overlay.DestroyOverlay();

    // Ensure threads are properly joined or detached
    if (cheatThread.joinable()) {
        cheatThread.join();
    }
}

int main()
{
    // Initialize the login system
   // initialize_login_system();
    // Only proceed with the cheat if login is successful
    initialize_cheat();

    return 0;
}
