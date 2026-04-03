// EditorUI.cpp
#include "EditorUI.h"
#include <cstdio>

// Font id used by the UI text.
// This gets set from outside once the font is loaded.
static s8 gUiFontId = 0;

// Cached window size.
// We store this so helper functions like DrawRect / PrintText
// can use the current window dimensions without passing them around every time.
static int gCachedWindowW = 1280;
static int gCachedWindowH = 720;

// Smaller text scale so the UI looks less bulky.
static constexpr float UI_TEXT_SCALE = 0.75f;

void EditorUI_SetFont(s8 fontId)
{
    // Save the font id so PrintText can use it later.
    gUiFontId = fontId;
}

// ── quad mesh ────────────────────────────────────────────────────────────────

// Reusable quad mesh for drawing UI rectangles.
// Instead of rebuilding boxes every frame, we make one unit quad and scale it.
static AEGfxVertexList* gUiQuad = nullptr;

static AEGfxVertexList* CreateUnitQuad()
{
    // Start building a mesh.
    AEGfxMeshStart();

    // First triangle of the quad.
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0, 1,
        0.5f, -0.5f, 0xFFFFFFFF, 1, 1,
        -0.5f, 0.5f, 0xFFFFFFFF, 0, 0
    );

    // Second triangle of the quad.
    AEGfxTriAdd(
        0.5f, -0.5f, 0xFFFFFFFF, 1, 1,
        0.5f, 0.5f, 0xFFFFFFFF, 1, 0,
        -0.5f, 0.5f, 0xFFFFFFFF, 0, 0
    );

    // Finish and return the mesh.
    return AEGfxMeshEnd();
}

void EditorUI_Init()
{
    // Build the shared quad once at startup.
    gUiQuad = CreateUnitQuad();
}

void EditorUI_Shutdown()
{
    // Free the mesh when shutting down to avoid memory leaks.
    if (gUiQuad)
    {
        AEGfxMeshFree(gUiQuad);
        gUiQuad = nullptr;
    }
}

// ── helpers ───────────────────────────────────────────────────────────────────

// Convert a top-left based y value into the engine's drawing y system.
// UI input usually comes from top-left origin, but rendering here works from bottom-left.
static inline float UIY(int winH, s32 topLeftY)
{
    return (float)(winH - topLeftY);
}

static bool PointInRect(float px, float py, float x, float y, float w, float h)
{
    // Basic rectangle hit test.
    // Returns true if the mouse point is inside the given box.
    return px >= x && px <= x + w && py >= y && py <= y + h;
}

static void DrawRect(float cx, float cy, float w, float h,
    float r, float g, float b, float a)
{
    // Put the camera in the center of the window so UI drawing stays stable.
    AEGfxSetCamPosition(gCachedWindowW * 0.5f, gCachedWindowH * 0.5f);

    // Build transform matrices:
    // scale -> rotation -> translation
    AEMtx33 sc, ro, tr, m;
    AEMtx33Rot(&ro, 0.f);        // no rotation
    AEMtx33Scale(&sc, w, h);     // scale unit quad to requested width/height
    AEMtx33Trans(&tr, cx, cy);   // move it into position

    // Combine them into one final transform matrix.
    AEMtx33Concat(&m, &ro, &sc);
    AEMtx33Concat(&m, &tr, &m);

    // Draw the quad as a colored rectangle.
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransform(m.m);
    AEGfxSetColorToMultiply(r, g, b, a);
    AEGfxMeshDraw(gUiQuad, AE_GFX_MDM_TRIANGLES);
}

static void PrintText(const char* text, float x, float y,
    float r, float g, float b, float a = 1.f)
{
    // Convert screen position into normalized device coordinates
    // because AEGfxPrint expects NDC instead of pixel coordinates.
    float ndcX = (x / (float)gCachedWindowW) * 2.f - 1.f;
    float ndcY = (y / (float)gCachedWindowH) * 2.f - 1.f;

    // Set text rendering state.
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetTransparency(1.f);

    // Draw the text.
    AEGfxPrint(gUiFontId, text, ndcX, ndcY, UI_TEXT_SCALE, r, g, b, a);
}

static void Sep(float x, float y, float w)
{
    // Draw a thin separator line across the panel.
    DrawRect(x + w * 0.5f, y + 1.f, w, 2.f, 0.08f, 0.08f, 0.08f, 1.f);
}

static bool Button(const char* label,
    float x, float y, float w, float h,
    float mx, float my, bool pressed,
    bool selected = false)
{
    // Check if mouse is currently over the button.
    bool hot = PointInRect(mx, my, x, y, w, h);

    // Default button color.
    float r = 0.18f, g = 0.18f, b = 0.18f, a = 0.92f;

    // If this button is the selected one, tint it greenish.
    if (selected)
    {
        r = 0.20f;
        g = 0.38f;
        b = 0.20f;
    }

    // If mouse is hovering, brighten it slightly.
    if (hot)
    {
        r += 0.08f;
        g += 0.08f;
        b += 0.08f;
    }

    // Draw the button body.
    DrawRect(x + w * 0.5f, y + h * 0.5f, w, h, r, g, b, a);

    // Draw the button label.
    PrintText(label, x + 8.f, y + (h * 0.5f) - 7.f, 1, 1, 1, 1);

    // Button is considered clicked only if mouse is over it and pressed this frame.
    return hot && pressed;
}

// ── main draw ─────────────────────────────────────────────────────────────────
void EditorUI_Draw(EditorUIState& ui, EditorUIIO& io,
    int windowW, int windowH,
    s32 mxTL, s32 myTL,
    bool mouseLPressed)
{
    // Update cached window size every frame.
    gCachedWindowW = windowW;
    gCachedWindowH = windowH;

    // By default, UI is not capturing input.
    io.mouseCaptured = false;
    io.keyboardCaptured = false;

    // Convert mouse position into the UI draw coordinate system.
    float mx = (float)mxTL;
    float my = UIY(windowH, myTL);

    // If mouse is inside the left UI panel, mark mouse as captured
    // so the rest of the game/editor knows UI is using the mouse.
    if (mx >= 0.0f && mx <= ui.panelW)
        io.mouseCaptured = true;

    // Common layout values.
    const float x = ui.pad;                    // left padding
    const float w = ui.panelW - ui.pad * 2.f; // usable width
    const float h = ui.rowH;                  // row height
    float y = (float)windowH - ui.pad - h;    // starting y from top

    // ── title ────────────────────────────────────────────────────────────────
    PrintText("LEVEL EDITOR", x, y + 10.f, 0.9f, 0.9f, 0.9f);

    // Move down after drawing title.
    y -= (h + ui.gap);

    // Separator below title.
    Sep(x, y + h + 2.f, w);
    y -= ui.gap;

    // ── play / stop ─────────────────────────────────────────────────────────
    const char* playLabel = ui.playMode ? "stop" : "play";

    // If clicked, request play toggle.
    if (Button(playLabel, x, y, w, h, mx, my, mouseLPressed))
        ui.requestTogglePlay = true;

    y -= (h + ui.gap);

    // Separator after play button.
    Sep(x, y + h + 2.f, w);
    y -= ui.gap;

    // If in play mode, do not show the editing controls below.
    if (ui.playMode)
    {
        // Restore default render states before returning.
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToAdd(0, 0, 0, 0);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        return;
    }

    // ── tool section ────────────────────────────────────────────────────────
    PrintText("tool", x, y + 10.f, 0.7f, 0.7f, 0.7f);
    y -= (h + ui.gap);

    // Split row into 3 equal tool buttons.
    float thirdW = (w - ui.gap * 2.f) / 3.f;

    // Paint tool = place selected tile/object.
    if (Button("paint", x, y, thirdW, h, mx, my, mouseLPressed, ui.tool == EditorTool::Paint))
        ui.tool = EditorTool::Paint;

    // Erase tool = remove tile/object.
    if (Button("erase", x + thirdW + ui.gap, y, thirdW, h, mx, my, mouseLPressed, ui.tool == EditorTool::Erase))
        ui.tool = EditorTool::Erase;

    // Bind tool = connect a pressure plate to a spike.
    // Flow is usually:
    // 1) click pressure plate
    // 2) click spike
    // Then that spike becomes controlled by that pressure plate.
    if (Button("bind", x + (thirdW + ui.gap) * 2.f, y, thirdW, h, mx, my, mouseLPressed, ui.tool == EditorTool::Bind))
        ui.tool = EditorTool::Bind;

    y -= (h + ui.gap);

    // Show instruction text only while in bind mode.
    if (ui.tool == EditorTool::Bind)
    {
        const char* bindMsg =
            (ui.bindSourceTrapId < 0)
            ? "bind: click pressure plate"
            : "bind: click spike to toggle";

        PrintText(bindMsg, x, y + 10.f, 1.f, 0.85f, 0.35f);
        y -= (h + ui.gap);
    }

    // Separator after tools.
    Sep(x, y + h + 2.f, w);
    y -= ui.gap;

    // ── palette section ─────────────────────────────────────────────────────
    PrintText("tile", x, y + 10.f, 0.7f, 0.7f, 0.7f);
    y -= (h + ui.gap);

    if (Button("ground surface", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::GroundSurface))
        ui.brush = EditorTile::GroundSurface;
    y -= (h + ui.gap);

    if (Button("ground body", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::GroundBody))
        ui.brush = EditorTile::GroundBody;
    y -= (h + ui.gap);

    if (Button("ground bottom", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::GroundBottom))
        ui.brush = EditorTile::GroundBottom;
    y -= (h + ui.gap);

    if (Button("platform", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Platform))
        ui.brush = EditorTile::Platform;
    y -= (h + ui.gap);

    if (Button("spike", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Spike))
        ui.brush = EditorTile::Spike;
    y -= (h + ui.gap);

    if (Button("pressure plate", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::PressurePlate))
        ui.brush = EditorTile::PressurePlate;
    y -= (h + ui.gap);

    if (Button("lava", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Lava))
        ui.brush = EditorTile::Lava;
    y -= (h + ui.gap);

    if (Button("enemy", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Enemy))
        ui.brush = EditorTile::Enemy;
    y -= (h + ui.gap);

    if (Button("spawn", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Spawn))
        ui.brush = EditorTile::Spawn;
    y -= (h + ui.gap);

    if (Button("vine", x, y, w, h, mx, my, mouseLPressed, ui.brush == EditorTile::Vine))
        ui.brush = EditorTile::Vine;
    y -= (h + ui.gap);

    // Extra enemy subtype buttons only appear when enemy brush is selected.
    if (ui.brush == EditorTile::Enemy)
    {
        float sw = (w - ui.gap) * 0.5f;
        float sh = h * 0.8f;

        // Druid preset.
        if (Button("druid", x, y, sw, sh, mx, my, mouseLPressed,
            ui.enemyPreset == EditorEnemyPreset::Druid))
            ui.enemyPreset = EditorEnemyPreset::Druid;

        // Skeleton preset.
        if (Button("skeleton", x + sw + ui.gap, y, sw, sh, mx, my, mouseLPressed,
            ui.enemyPreset == EditorEnemyPreset::Skeleton))
            ui.enemyPreset = EditorEnemyPreset::Skeleton;

        // Boss preset.
        // Note: this x positioning is wider than the normal 2-button layout,
        // so make sure that is intentional for your panel width.
        if (Button("boss", x + (sw + ui.gap) * 2.2f, y, sw, sh, mx, my, mouseLPressed,
            ui.enemyPreset == EditorEnemyPreset::Boss))
            ui.enemyPreset = EditorEnemyPreset::Boss;

        y -= (sh + ui.gap);
    }

    // Separator after palette.
    Sep(x, y + h + 2.f, w);
    y -= ui.gap;

    // ── options section ──────────────────────────────────────────────────────

    // Toggle drag painting.
    if (Button(ui.dragPaint ? "drag: on" : "drag: off", x, y, w, h, mx, my, mouseLPressed))
        ui.dragPaint = !ui.dragPaint;
    y -= (h + ui.gap);

    // Toggle grid visibility.
    if (Button(ui.showGrid ? "grid: on" : "grid: off", x, y, w, h, mx, my, mouseLPressed))
        ui.showGrid = !ui.showGrid;
    y -= (h + ui.gap);

    // Separator after options.
    Sep(x, y + h + 2.f, w);
    y -= ui.gap;

    // ── file section ─────────────────────────────────────────────────────────
    PrintText("file", x, y + 10.f, 0.7f, 0.7f, 0.7f);
    y -= (h + ui.gap);

    // Request save.
    if (Button("save", x, y, w, h, mx, my, mouseLPressed))
        ui.requestSave = true;
    y -= (h + ui.gap);

    // Request load.
    if (Button("load", x, y, w, h, mx, my, mouseLPressed))
        ui.requestLoad = true;
    y -= (h + ui.gap);

    // Request full map clear.
    if (Button("clear map", x, y, w, h, mx, my, mouseLPressed))
        ui.requestClearMap = true;

    // Restore default graphics state after UI finishes drawing.
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
}