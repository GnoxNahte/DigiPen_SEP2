#include "MainMenu.h"
#include "AEEngine.h"

#include <string>
#include <filesystem>
#include <cstdio>

namespace
{
    // change this to your actual menu background asset
    // if it fails to load, menu still works (just solid color)
    static const char* MENU_BG_PATH = "Assets/menu_background.png";

    static bool gRequestStart = false;
    static bool gRequestQuit = false;
    static std::string gStartPath;

    static s8 gFont = -1;

    static AEGfxVertexList* gQuad = nullptr;
    static AEGfxTexture* gBgTex = nullptr;

    static AEGfxVertexList* CreateUnitQuad()
    {
        AEGfxMeshStart();

        AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 1,
            0.5f, -0.5f, 0xFFFFFFFF, 1, 1,
            -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);

        AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1, 1,
            0.5f, 0.5f, 0xFFFFFFFF, 1, 0,
            -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);

        return AEGfxMeshEnd();
    }

    static float ConvertY_TopLeftToBottomLeft(int winH, int mouseYTopLeft)
    {
        return (float)(winH - mouseYTopLeft);
    }

    static bool PointInRect(float px, float py, float x, float y, float w, float h)
    {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }

    static void SetScreenSpaceCamera(int winW, int winH)
    {
        // screen-space: treat positions as pixels.
        // center camera at (w/2, h/2) so (0..w, 0..h) maps nicely.
        AEGfxSetCamPosition(winW * 0.5f, winH * 0.5f);
    }

    static void DrawText(const char* text, float xPx, float yPx, float scale,
        float r, float g, float b, float a,
        int winW, int winH)
    {
        if (gFont < 0) return;

        // AEGfxPrint uses NDC [-1..1]
        const float ndcX = (xPx / (float)winW) * 2.f - 1.f;
        const float ndcY = (yPx / (float)winH) * 2.f - 1.f;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToAdd(0, 0, 0, 0);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxSetTransparency(1.f);

        AEGfxPrint(gFont, text, ndcX, ndcY, scale, r, g, b, a);
    }

    static void DrawRect(float x, float y, float w, float h,
        float r, float g, float b, float a,
        int winW, int winH)
    {
        SetScreenSpaceCamera(winW, winH);

        AEMtx33 sc, ro, tr, m;
        AEMtx33Rot(&ro, 0.f);
        AEMtx33Scale(&sc, w, h);
        AEMtx33Trans(&tr, x + w * 0.5f, y + h * 0.5f);
        AEMtx33Concat(&m, &ro, &sc);
        AEMtx33Concat(&m, &tr, &m);

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransform(m.m);
        AEGfxSetColorToMultiply(r, g, b, a);
        AEGfxMeshDraw(gQuad, AE_GFX_MDM_TRIANGLES);
    }

    static void DrawBackgroundCentered(int winW, int winH)
    {
        if (!gBgTex) return;

        SetScreenSpaceCamera(winW, winH);

        // full screen quad centered at (w/2, h/2)
        AEMtx33 sc, ro, tr, m;
        AEMtx33Rot(&ro, 0.f);
        AEMtx33Scale(&sc, (float)winW, (float)winH);
        AEMtx33Trans(&tr, winW * 0.5f, winH * 0.5f);
        AEMtx33Concat(&m, &ro, &sc);
        AEMtx33Concat(&m, &tr, &m);

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransform(m.m);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxSetTransparency(1.f);

        AEGfxTextureSet(gBgTex, 0, 0);
        AEGfxMeshDraw(gQuad, AE_GFX_MDM_TRIANGLES);
    }

    static bool Button(const char* label,
        float x, float y, float w, float h,
        float mx, float my,
        bool mouseTriggered,
        int winW, int winH)
    {
        const bool hot = PointInRect(mx, my, x, y, w, h);

        // brighter for visibility
        float br = 0.22f, bg = 0.22f, bb = 0.22f;
        if (hot) { br = 0.34f; bg = 0.46f; bb = 0.34f; }

        DrawRect(x, y, w, h, br, bg, bb, 0.92f, winW, winH);

        // slightly inset label
        DrawText(label, x + 16.f, y + h * 0.5f - 6.f, 0.8f, 1, 1, 1, 1, winW, winH);

        return hot && mouseTriggered;
    }
}

namespace MainMenu
{
    void Init()
    {
        if (!gQuad) gQuad = CreateUnitQuad();

        // smaller font
        int fontId = AEGfxCreateFont("Assets/buggy-font.ttf", 16);
        if (fontId < 0) fontId = AEGfxCreateFont("../Assets/buggy-font.ttf", 16);
        if (fontId < 0) fontId = AEGfxCreateFont("../../Assets/buggy-font.ttf", 16);
        gFont = (s8)fontId;

        // background texture (optional)
        gBgTex = AEGfxTextureLoad(MENU_BG_PATH);

        gRequestStart = false;
        gRequestQuit = false;
        gStartPath.clear();
    }

    void Shutdown()
    {
        if (gBgTex) { AEGfxTextureUnload(gBgTex); gBgTex = nullptr; }

        if (gQuad) { AEGfxMeshFree(gQuad); gQuad = nullptr; }

        if (gFont >= 0)
        {
            AEGfxDestroyFont(gFont);
            gFont = -1;
        }

        gRequestStart = false;
        gRequestQuit = false;
        gStartPath.clear();
    }

    void Update(int w, int h, int mouseX, int mouseY, bool mouseTriggered)
    {
        const float mx = (float)mouseX;
        const float my = ConvertY_TopLeftToBottomLeft(h, mouseY);

        // layout: left column
        const float leftX = 80.f;
        const float btnW = 320.f;
        const float btnH = 50.f;
        float y = h - 240.f;

        // level editor
        if (Button("level editor", leftX, y, btnW, btnH, mx, my, mouseTriggered, w, h))
        {
            gRequestStart = true;
            gStartPath = "editor"; // consumed by MainMenuScene to route to GS_LEVEL_EDITOR
        }

        y -= 70.f;
        Button("settings (todo)", leftX, y, btnW, btnH, mx, my, false, w, h);

        y -= 70.f;
        Button("credits (todo)", leftX, y, btnW, btnH, mx, my, false, w, h);

        y -= 70.f;
        if (Button("quit", leftX, y, btnW, btnH, mx, my, mouseTriggered, w, h))
        {
            gRequestQuit = true;
        }
    }

    void Render(int w, int h)
    {
        // base clear (in case background fails)
        AEGfxSetBackgroundColor(0.05f, 0.05f, 0.05f);

        // background image (centered)
        DrawBackgroundCentered(w, h);

        // optional dark overlay so buttons pop
        DrawRect(0.f, 0.f, (float)w, (float)h, 0.f, 0.f, 0.f, 0.35f, w, h);

        // title
        DrawText("GAME TITLE", 80.f, h - 120.f, 1.0f, 1, 1, 1, 1, w, h);
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

    // not used by this simplified menu (kept to satisfy header)
    int GetSelectedLevelIndex() { return 1; }
    void SetSelectedLevelIndex(int) {}
}