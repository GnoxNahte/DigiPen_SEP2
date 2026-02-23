#include "EditorUI.hpp"

static s8 gUiFontId = -1;
static constexpr float UI_TEXT_SCALE = 1.0f;

void EditorUI_SetFont(s8 fontId)
{
    gUiFontId = fontId;
}

static AEGfxVertexList* gUiQuad = nullptr;

static AEGfxVertexList* CreateUnitQuad(u32 color)
{
    AEGfxMeshStart();

    AEGfxTriAdd(-0.5f, -0.5f, color, 0.0f, 1.0f,
        0.5f, -0.5f, color, 1.0f, 1.0f,
        -0.5f, 0.5f, color, 0.0f, 0.0f);

    AEGfxTriAdd(0.5f, -0.5f, color, 1.0f, 1.0f,
        0.5f, 0.5f, color, 1.0f, 0.0f,
        -0.5f, 0.5f, color, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}

void EditorUI_Init()
{
    gUiQuad = CreateUnitQuad(0xFFFFFFFF);
}

void EditorUI_Shutdown()
{
    if (gUiQuad)
    {
        AEGfxMeshFree(gUiQuad);
        gUiQuad = nullptr;
    }
}

static inline float ScreenY_To_UIY(int windowH, s32 myTopLeft)
{
    return (float)(windowH - myTopLeft);
}

static bool PointInRect(float px, float py, float x, float y, float w, float h)
{
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

static void DrawRect(float cx, float cy, float w, float h, float r, float g, float b, float a)
{
    AEMtx33 scale, rot, trans, m;
    AEMtx33Rot(&rot, 0.0f);
    AEMtx33Scale(&scale, w, h);
    AEMtx33Trans(&trans, cx, cy);

    AEMtx33Concat(&m, &rot, &scale);
    AEMtx33Concat(&m, &trans, &m);

    AEGfxSetTransform(m.m);
    AEGfxSetColorToMultiply(r, g, b, a);
    AEGfxMeshDraw(gUiQuad, AE_GFX_MDM_TRIANGLES);
}

static inline float PxToNdcX(float px, float w) { return (px / (w * 0.5f)) - 1.0f; }
static inline float PxToNdcY(float py, float h) { return (py / (h * 0.5f)) - 1.0f; }

static void PrintTextPx(const char* text, float xPx, float yPx, float scale,
    float r, float g, float b, float a)
{
    if (gUiFontId < 0 || !text) return;

    const float w = (float)AEGfxGetWindowWidth();
    const float h = (float)AEGfxGetWindowHeight();

    const float x = PxToNdcX(xPx, w);
    const float y = PxToNdcY(yPx, h);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEGfxPrint(gUiFontId, text, x, y, scale, r, g, b, a);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
}

static bool Button(const char* label,
    float x, float y, float w, float h,
    float mx, float my, bool pressed,
    bool selected = false)
{
    const bool hot = PointInRect(mx, my, x, y, w, h);

    float r = 0.18f, g = 0.18f, b = 0.18f, a = 0.90f;
    if (selected) { r = 0.20f; g = 0.35f; b = 0.20f; }
    if (hot) { r += 0.08f; g += 0.08f; b += 0.08f; }

    DrawRect(x + w * 0.5f, y + h * 0.5f, w, h, r, g, b, a);
    PrintTextPx(label, x + 10.0f, y + (h * 0.5f) - 6.0f, UI_TEXT_SCALE, 1, 1, 1, 1);

    return hot && pressed;
}

static void SeparatorLine(float x, float y, float w)
{
    DrawRect(x + w * 0.5f, y + 1.0f, w, 2.0f, 0.10f, 0.10f, 0.10f, 1.0f);
}

void EditorUI_Draw(EditorUIState& ui, EditorUIIO& io,
    int windowW, int windowH,
    s32 mxTL, s32 myTL,
    bool mouseLPressed)
{
    // reset one-frame actions
    ui.requestResetPlayer = false;
    ui.requestClearMap = false;
    ui.requestSave = false;
    ui.requestLoad = false;

    io.mouseCaptured = false;
    io.keyboardCaptured = false;

    const float mx = (float)mxTL;
    const float my = ScreenY_To_UIY(windowH, myTL);

    // ui camera
    AEGfxSetCamPosition(windowW * 0.5f, windowH * 0.5f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // panel bg
    DrawRect(ui.panelW * 0.5f, windowH * 0.5f, ui.panelW, (float)windowH,
        0.12f, 0.12f, 0.12f, 0.95f);

    if (PointInRect(mx, my, 0.0f, 0.0f, ui.panelW, (float)windowH))
        io.mouseCaptured = true;

    float x = ui.pad;
    float y = (float)windowH - ui.pad - ui.rowH;
    float w = ui.panelW - ui.pad * 2.0f;
    float h = ui.rowH;

    PrintTextPx("level editor", x, y + 12.0f, UI_TEXT_SCALE, 1, 1, 1, 1);
    y -= (ui.rowH + ui.gap);
    SeparatorLine(x, y + ui.rowH + 2.0f, w);
    y -= ui.gap;

    if (!ui.playMode)
    {
        if (Button("play", x, y, w, h, mx, my, mouseLPressed))
            ui.playMode = true;
    }
    else
    {
        if (Button("stop", x, y, w, h, mx, my, mouseLPressed))
            ui.playMode = false;

        y -= (ui.rowH + ui.gap);
        if (Button("reset player", x, y, w, h, mx, my, mouseLPressed))
            ui.requestResetPlayer = true;
    }
    y -= (ui.rowH + ui.gap);

    if (Button("clear map", x, y, w, h, mx, my, mouseLPressed))
        ui.requestClearMap = true;
    y -= (ui.rowH + ui.gap);

    if (Button("save", x, y, w, h, mx, my, mouseLPressed))
        ui.requestSave = true;
    y -= (ui.rowH + ui.gap);

    if (Button("load", x, y, w, h, mx, my, mouseLPressed))
        ui.requestLoad = true;
    y -= (ui.rowH + ui.gap);

    SeparatorLine(x, y + ui.rowH + 2.0f, w);
    y -= ui.gap;

    PrintTextPx("tools", x, y + 12.0f, UI_TEXT_SCALE, 0.85f, 0.85f, 0.85f, 1);
    y -= (ui.rowH + ui.gap);

    const float halfW = (w - ui.gap) * 0.5f;

    if (Button("paint", x, y, halfW, h, mx, my, mouseLPressed, ui.tool == EditorTool::Paint))
        ui.tool = EditorTool::Paint;
    if (Button("erase", x + halfW + ui.gap, y, halfW, h, mx, my, mouseLPressed, ui.tool == EditorTool::Erase))
        ui.tool = EditorTool::Erase;
    y -= (ui.rowH + ui.gap);

    if (Button("fill", x, y, halfW, h, mx, my, mouseLPressed, ui.tool == EditorTool::Fill))
        ui.tool = EditorTool::Fill;
    if (Button("select", x + halfW + ui.gap, y, halfW, h, mx, my, mouseLPressed, ui.tool == EditorTool::Select))
        ui.tool = EditorTool::Select;
    y -= (ui.rowH + ui.gap);

    SeparatorLine(x, y + ui.rowH + 2.0f, w);
    y -= ui.gap;

    PrintTextPx("palette", x, y + 12.0f, UI_TEXT_SCALE, 0.85f, 0.85f, 0.85f, 1);
    y -= (ui.rowH + ui.gap);

    if (Button("eraser (empty)", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Empty))
        ui.brush = EditorTile::Empty;
    y -= (ui.rowH + ui.gap);

    if (Button("ground", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Ground))
        ui.brush = EditorTile::Ground;
    y -= (ui.rowH + ui.gap);

    if (Button("spike", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Spike))
        ui.brush = EditorTile::Spike;
    y -= (ui.rowH + ui.gap);

    SeparatorLine(x, y + ui.rowH + 2.0f, w);
    y -= ui.gap;

    if (Button(ui.dragPaint ? "drag paint: on" : "drag paint: off", x, y, w, h, mx, my, mouseLPressed))
        ui.dragPaint = !ui.dragPaint;
    y -= (ui.rowH + ui.gap);

    if (Button(ui.showGrid ? "grid: on" : "grid: off", x, y, w, h, mx, my, mouseLPressed))
        ui.showGrid = !ui.showGrid;
    y -= (ui.rowH + ui.gap);

    if (Button(ui.snapToGrid ? "snap: on" : "snap: off", x, y, w, h, mx, my, mouseLPressed))
        ui.snapToGrid = !ui.snapToGrid;
    y -= (ui.rowH + ui.gap);

    if (Button(ui.showDebug ? "debug: on" : "debug: off", x, y, w, h, mx, my, mouseLPressed))
        ui.showDebug = !ui.showDebug;
}