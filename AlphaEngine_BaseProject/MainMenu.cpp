
#include "MainMenu.h"
#include "AEEngine.h"

#include <vector>
#include <string>
#include <filesystem>
#include <cstdio>
#include <algorithm>

namespace
{
    struct LevelEntry
    {
        int index = 1;
        std::string path;
        std::string label;
    };

    static std::vector<LevelEntry> gLevels;

    static int  gSelected = 0;
    static bool gRequestStart = false;
    static bool gRequestQuit = false;
    static std::string gStartPath;

    static s8 gFont = -1;
    static AEGfxVertexList* gQuad = nullptr;

    static AEGfxVertexList* CreateUnitQuad()
    {
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 1, 0.5f, -0.5f, 0xFFFFFFFF, 1, 1, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
        AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1, 1, 0.5f, 0.5f, 0xFFFFFFFF, 1, 0, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
        return AEGfxMeshEnd();
    }

    static void DrawRect(float cx, float cy, float w, float h, float r, float g, float b, float a, int winW, int winH)
    {
        // screen-space camera
        AEGfxSetCamPosition(winW * 0.5f, winH * 0.5f);

        AEMtx33 sc, ro, tr, m;
        AEMtx33Rot(&ro, 0.f);
        AEMtx33Scale(&sc, w, h);
        AEMtx33Trans(&tr, cx, cy);
        AEMtx33Concat(&m, &ro, &sc);
        AEMtx33Concat(&m, &tr, &m);

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransform(m.m);
        AEGfxSetColorToMultiply(r, g, b, a);
        AEGfxMeshDraw(gQuad, AE_GFX_MDM_TRIANGLES);
    }

    static void PrintText(const char* text, float x, float y, float scale, float r, float g, float b, float a, int winW, int winH)
    {
        if (gFont < 0) return;

        // AEGfxPrint expects NDC coords [-1..1]
        float ndcX = (x / (float)winW) * 2.f - 1.f;
        float ndcY = (y / (float)winH) * 2.f - 1.f;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToAdd(0, 0, 0, 0);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxSetTransparency(1.f);

        AEGfxPrint(gFont, text, ndcX, ndcY, scale, r, g, b, a);
    }

    static bool PointInRect(float px, float py, float x, float y, float w, float h)
    {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }

    static bool Button(const char* label, float x, float y, float w, float h,
        float mx, float my, bool mouseLTriggered,
        bool selected, int winW, int winH)
    {
        const bool hot = PointInRect(mx, my, x, y, w, h);

        float r = 0.15f, g = 0.15f, b = 0.15f, a = 0.95f;
        if (selected) { r = 0.18f; g = 0.35f; b = 0.18f; }
        if (hot) { r += 0.08f; g += 0.08f; b += 0.08f; }

        DrawRect(x + w * 0.5f, y + h * 0.5f, w, h, r, g, b, a, winW, winH);
        PrintText(label, x + 10.f, y + (h * 0.5f) - 7.f, 1.0f, 1, 1, 1, 1, winW, winH);

        return hot && mouseLTriggered;
    }

    static std::string MakeLevelPath(int idx)
    {
        char buf[64];
        sprintf_s(buf, "level%02d.lvl", idx);
        return std::string(buf);
    }

    static void RefreshLevelList()
    {
        gLevels.clear();

        // scan level01..level99 (simple & robust)
        for (int i = 1; i <= 99; ++i)
        {
            std::string path = MakeLevelPath(i);
            if (std::filesystem::exists(path))
            {
                LevelEntry e;
                e.index = i;
                e.path = path;

                char label[64];
                sprintf_s(label, "level %02d", i);
                e.label = label;

                gLevels.push_back(e);
            }
        }

        // if none exist, still offer level01 as a default target
        if (gLevels.empty())
        {
            LevelEntry e;
            e.index = 1;
            e.path = MakeLevelPath(1);
            e.label = "level 01 (new)";
            gLevels.push_back(e);
        }

        gSelected = std::clamp(gSelected, 0, (int)gLevels.size() - 1);
    }

    static float UIY(int winH, int mouseYTopLeft)
    {
        // your input mouseY is top-left origin. convert to bottom-left UI space
        return (float)(winH - mouseYTopLeft);
    }
}

namespace MainMenu
{
    void Init()
    {
        if (!gQuad) gQuad = CreateUnitQuad();

        // load any font you have (use same paths you used in editor)
        int fontId = AEGfxCreateFont("Assets/buggy-font.ttf", 24);
        if (fontId < 0) fontId = AEGfxCreateFont("../Assets/buggy-font.ttf", 24);
        if (fontId < 0) fontId = AEGfxCreateFont("../../Assets/buggy-font.ttf", 24);
        gFont = (s8)fontId;

        gRequestStart = false;
        gRequestQuit = false;
        gStartPath.clear();

        RefreshLevelList();
    }

    void Shutdown()
    {
        if (gQuad) { AEGfxMeshFree(gQuad); gQuad = nullptr; }
        if (gFont >= 0) { AEGfxDestroyFont(gFont); gFont = -1; }

        gLevels.clear();
        gSelected = 0;
        gRequestStart = false;
        gRequestQuit = false;
        gStartPath.clear();
    }

    void Update(int windowW, int windowH, int mouseX, int mouseY, bool mouseLTriggered)
    {
        (void)windowW;

        // quick keyboard shortcuts
        if (AEInputCheckTriggered(AEVK_UP))   gSelected = std::max(0, gSelected - 1);
        if (AEInputCheckTriggered(AEVK_DOWN)) gSelected = std::min((int)gLevels.size() - 1, gSelected + 1);

        if (AEInputCheckTriggered(AEVK_F5))
            RefreshLevelList();

        // mouse state
        float mx = (float)mouseX;
        float my = UIY(windowH, mouseY);

        // layout (matches your sketch: big title left, buttons left, level list right)
        const float pad = 30.f;

        const float leftX = pad;
        const float leftW = 260.f;

        const float listX = windowW - pad - 320.f;
        const float listW = 320.f;

        const float btnH = 44.f;
        const float btnGap = 14.f;

        float y = windowH - pad - 140.f;

        // play
        if (Button("play", leftX, y, leftW, btnH, mx, my, mouseLTriggered, false, windowW, windowH))
        {
            gRequestStart = true;
            gStartPath = gLevels[gSelected].path;
        }

        y -= (btnH + btnGap);

        // settings (placeholder)
        Button("settings (todo)", leftX, y, leftW, btnH, mx, my, false, false, windowW, windowH);
        y -= (btnH + btnGap);

        // credits (placeholder)
        Button("credits (todo)", leftX, y, leftW, btnH, mx, my, false, false, windowW, windowH);
        y -= (btnH + btnGap);

        // quit
        if (Button("quit", leftX, y, leftW, btnH, mx, my, mouseLTriggered, false, windowW, windowH))
        {
            gRequestQuit = true;
        }

        // level list buttons (right)
        float listY = windowH - pad - 220.f;
        const float rowH = 38.f;
        const float rowGap = 10.f;

        for (int i = 0; i < (int)gLevels.size(); ++i)
        {
            bool isSel = (i == gSelected);
            if (Button(gLevels[i].label.c_str(), listX, listY, listW, rowH, mx, my, mouseLTriggered, isSel, windowW, windowH))
                gSelected = i;

            listY -= (rowH + rowGap);

            // stop after a reasonable number visible
            if (listY < pad) break;
        }
    }

    void Render(int windowW, int windowH)
    {
        AEGfxSetBackgroundColor(0.05f, 0.05f, 0.05f);

        // background frame
        DrawRect(windowW * 0.5f, windowH * 0.5f, (float)windowW - 40.f, (float)windowH - 40.f,
            0.08f, 0.08f, 0.08f, 1.0f, windowW, windowH);

        // title
        PrintText("game title", 60.f, windowH - 90.f, 2.0f, 0.9f, 0.9f, 0.9f, 1.f, windowW, windowH);

        // controls hint
        PrintText("controls:", 360.f, windowH - 220.f, 1.0f, 0.9f, 0.7f, 0.2f, 1.f, windowW, windowH);
        PrintText("wasd / arrow keys: move", 360.f, windowH - 250.f, 1.0f, 0.9f, 0.7f, 0.2f, 1.f, windowW, windowH);
        PrintText("space: jump", 360.f, windowH - 280.f, 1.0f, 0.9f, 0.7f, 0.2f, 1.f, windowW, windowH);

        // right panel label
        PrintText("levels", windowW - 330.f, windowH - 170.f, 1.2f, 0.9f, 0.7f, 0.2f, 1.f, windowW, windowH);

        // footer hint
        PrintText("f5: refresh levels", 60.f, 50.f, 0.9f, 0.6f, 0.6f, 0.6f, 1.f, windowW, windowH);
    }

    bool ConsumeStartRequest(std::string& outPath)
    {
        if (!gRequestStart) return false;
        gRequestStart = false;
        outPath = gStartPath;
        gStartPath.clear();
        return true;
    }

    bool ConsumeQuitRequest()
    {
        if (!gRequestQuit) return false;
        gRequestQuit = false;
        return true;
    }

    int GetSelectedLevelIndex()
    {
        if (gLevels.empty()) return 1;
        return gLevels[gSelected].index;
    }

    void SetSelectedLevelIndex(int idx)
    {
        if (gLevels.empty()) return;

        // find matching entry, else clamp to first
        for (int i = 0; i < (int)gLevels.size(); ++i)
        {
            if (gLevels[i].index == idx) { gSelected = i; return; }
        }
        gSelected = 0;
    }
}