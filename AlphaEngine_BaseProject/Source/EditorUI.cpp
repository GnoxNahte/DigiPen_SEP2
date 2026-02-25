// EditorUI.cpp
#include "EditorUI.h"
#include <cstdio>

static s8  gUiFontId = 0;
static int gCachedWindowW = 1280;
static int gCachedWindowH = 720;

static constexpr float UI_TEXT_SCALE = 1.0f;

void EditorUI_SetFont(s8 fontId) { gUiFontId = fontId; }

// ── quad mesh ────────────────────────────────────────────────────────────────
static AEGfxVertexList* gUiQuad = nullptr;

static AEGfxVertexList* CreateUnitQuad()
{
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 1, 0.5f, -0.5f, 0xFFFFFFFF, 1, 1, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1, 1, 0.5f, 0.5f, 0xFFFFFFFF, 1, 0, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    return AEGfxMeshEnd();
}

void EditorUI_Init() { gUiQuad = CreateUnitQuad(); }
void EditorUI_Shutdown() { if (gUiQuad) { AEGfxMeshFree(gUiQuad); gUiQuad = nullptr; } }

// ── helpers ───────────────────────────────────────────────────────────────────
static inline float UIY(int winH, s32 topLeftY) { return (float)(winH - topLeftY); }

static bool PointInRect(float px, float py, float x, float y, float w, float h)
{
    return px >= x && px <= x + w && py >= y && py <= y + h;
}

static void DrawRect(float cx, float cy, float w, float h,
    float r, float g, float b, float a)
{
    AEGfxSetCamPosition(gCachedWindowW * 0.5f, gCachedWindowH * 0.5f);

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
    AEGfxMeshDraw(gUiQuad, AE_GFX_MDM_TRIANGLES);
}

static void PrintText(const char* text, float x, float y,
    float r, float g, float b, float a = 1.f)
{
    float ndcX = (x / (float)gCachedWindowW) * 2.f - 1.f;
    float ndcY = (y / (float)gCachedWindowH) * 2.f - 1.f;

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetTransparency(1.f);
    AEGfxPrint(gUiFontId, text, ndcX, ndcY, UI_TEXT_SCALE, r, g, b, a);
}

static void Sep(float x, float y, float w)
{
    DrawRect(x + w * 0.5f, y + 1.f, w, 2.f, 0.08f, 0.08f, 0.08f, 1.f);
}

static bool Button(const char* label,
    float x, float y, float w, float h,
    float mx, float my, bool pressed,
    bool selected = false)
{
    bool hot = PointInRect(mx, my, x, y, w, h);

    float r = 0.18f, g = 0.18f, b = 0.18f, a = 0.92f;
    if (selected) { r = 0.20f; g = 0.38f; b = 0.20f; }
    if (hot) { r += 0.08f; g += 0.08f; b += 0.08f; }

    DrawRect(x + w * 0.5f, y + h * 0.5f, w, h, r, g, b, a);
    PrintText(label, x + 8.f, y + (h * 0.5f) - 7.f, 1, 1, 1, 1);

    return hot && pressed;
}

// ── main draw ─────────────────────────────────────────────────────────────────
void EditorUI_Draw(EditorUIState& ui, EditorUIIO& io,
    int windowW, int windowH,
    s32 mxTL, s32 myTL,
    bool mouseLPressed)
{
    gCachedWindowW = windowW;
    gCachedWindowH = windowH;

    io.mouseCaptured = false;
    io.keyboardCaptured = false;

    float mx = (float)mxTL;
    float my = UIY(windowH, myTL);



    const float x = ui.pad;
    const float w = ui.panelW - ui.pad * 2.f;
    const float h = ui.rowH;
    float y = (float)windowH - ui.pad - h;

    // ── title ────────────────────────────────────────────────────────────────
    PrintText("LEVEL EDITOR", x, y + 10.f, 0.9f, 0.9f, 0.9f);
    y -= (h + ui.gap);
    Sep(x, y + h + 2.f, w);
    y -= ui.gap;

    // ── play / stop ──────────────────────────────────────────────────────────
    const char* playLabel = ui.playMode ? "stop" : "play";
    if (Button(playLabel, x, y, w, h, mx, my, mouseLPressed))
        ui.requestTogglePlay = true;
    y -= (h + ui.gap);
    Sep(x, y + h + 2.f, w);
    y -= ui.gap;

    // hide editing tools in play mode
    if (ui.playMode)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToAdd(0, 0, 0, 0);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        return;
    }

    // ── tool ─────────────────────────────────────────────────────────────────
    PrintText("tool", x, y + 10.f, 0.7f, 0.7f, 0.7f);
    y -= (h + ui.gap);

    float hw = (w - ui.gap) * 0.5f;
    if (Button("paint", x, y, hw, h, mx, my, mouseLPressed, ui.tool == EditorTool::Paint))
        ui.tool = EditorTool::Paint;
    if (Button("erase", x + hw + ui.gap, y, hw, h, mx, my, mouseLPressed, ui.tool == EditorTool::Erase))
        ui.tool = EditorTool::Erase;
    y -= (h + ui.gap);
    Sep(x, y + h + 2.f, w);
    y -= ui.gap;

    // ── palette ──────────────────────────────────────────────────────────────
    PrintText("tile", x, y + 10.f, 0.7f, 0.7f, 0.7f);
    y -= (h + ui.gap);

    if (Button("ground", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Ground))
        ui.brush = EditorTile::Ground;
    y -= (h + ui.gap);

    if (Button("spike", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Spike))
        ui.brush = EditorTile::Spike;
    y -= (h + ui.gap);

    if (Button("pressure plate", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::PressurePlate))
        ui.brush = EditorTile::PressurePlate;
    y -= (h + ui.gap);

    if (Button("enemy", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Enemy))
        ui.brush = EditorTile::Enemy;
    y -= (h + ui.gap);

    if (ui.brush == EditorTile::Enemy)
    {
        float sw = (w - ui.gap) * 0.5f;
        float sh = h * 0.8f;
        if (Button("druid", x, y, sw, sh, mx, my, mouseLPressed,
            ui.enemyPreset == EditorEnemyPreset::Druid))
            ui.enemyPreset = EditorEnemyPreset::Druid;
        if (Button("skeleton", x + sw + ui.gap, y, sw, sh, mx, my, mouseLPressed,
            ui.enemyPreset == EditorEnemyPreset::Skeleton))
            ui.enemyPreset = EditorEnemyPreset::Skeleton;
        y -= (sh + ui.gap);
    }

    Sep(x, y + h + 2.f, w);
    y -= ui.gap;

    // ── options ──────────────────────────────────────────────────────────────
    if (Button(ui.dragPaint ? "drag: on" : "drag: off", x, y, w, h, mx, my, mouseLPressed))
        ui.dragPaint = !ui.dragPaint;
    y -= (h + ui.gap);

    if (Button(ui.showGrid ? "grid: on" : "grid: off", x, y, w, h, mx, my, mouseLPressed))
        ui.showGrid = !ui.showGrid;
    y -= (h + ui.gap);

    Sep(x, y + h + 2.f, w);
    y -= ui.gap;

    // ── file ─────────────────────────────────────────────────────────────────
    PrintText("file", x, y + 10.f, 0.7f, 0.7f, 0.7f);
    y -= (h + ui.gap);

    if (Button("save", x, y, w, h, mx, my, mouseLPressed))
        ui.requestSave = true;
    y -= (h + ui.gap);

    if (Button("load", x, y, w, h, mx, my, mouseLPressed))
        ui.requestLoad = true;
    y -= (h + ui.gap);

    if (Button("clear map", x, y, w, h, mx, my, mouseLPressed))
        ui.requestClearMap = true;

    // restore state
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
}