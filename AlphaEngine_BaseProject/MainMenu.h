#pragma once

#include <string>

// simple immediate-mode main menu for selecting a saved level file
namespace MainMenu
{
    void Init();
    void Shutdown();

    // call every frame
    void Update(int windowW, int windowH, int mouseX, int mouseY, bool mouseLTriggered);
    void Render(int windowW, int windowH);

    // returns true once when user presses play. fills outPath with chosen level file.
    bool ConsumeStartRequest(std::string& outPath);

    // returns true once when user presses quit.
    bool ConsumeQuitRequest();

    // optional: for debugging
    int  GetSelectedLevelIndex();
    void SetSelectedLevelIndex(int idx);
}