# Level Editor / Level File README

This README explains the custom level-editing pipeline used by these files, in the order a teammate should understand them:

`MapTile -> MapGrid -> LevelEditor -> EditorUI -> LevelIO`

The goal is to make it easier for anyone on the shared Git project to extend the editor without getting lost.

---

## File Flow Overview

### MapTile
`MapTile` is the most basic unit in the system. It defines the tile types that can exist on the map.

Current tile types:
- `NONE`
- `GROUND_SURFACE`
- `GROUND_BODY`
- `GROUND_BOTTOM`
- `PLATFORM`

If you want to add a brand new paintable tile type, this is the first place to edit.

---

### MapGrid
`MapGrid` owns the actual 2D map data. It is responsible for:
- storing tiles in a grid
- rendering tiles
- setting and getting tiles
- basic tile collision

In short, `MapGrid` is the actual map storage, drawing, and collision layer.

---

### LevelEditor
`LevelEditor` is where editing actually happens. It creates the map, camera, editor state, trap list, enemy list, spawn point, and save/load prompt state.

This file is the main bridge between UI choice and world result:
- if the selected brush is a tile, it places a tile into `MapGrid`
- if the selected brush is a trap, it stores a trap definition
- if the selected brush is an enemy, it stores an enemy definition
- if the selected brush is spawn, it updates the player spawn
- if the selected brush is vine, it stores a vine position

In short, `LevelEditor` is the gameplay-side controller of the editor. It decides what happens when the user clicks on the map.

---

### EditorUI
`EditorUI` is the front-end selector. It defines the editor tools and brushes that the panel can select.

Current brush types include:
- GroundSurface
- GroundBody
- GroundBottom
- Platform
- Spike
- PressurePlate
- Enemy
- Spawn
- Vine

Current enemy presets:
- Druid
- Skeleton

In short, `EditorUI` lets the user choose what they want to place, but it does not place anything by itself.

---

### LevelIO
`LevelIO` is the save/load layer.

It converts the current editor state into serializable level data and writes it to a `.lvl` file. It also loads a `.lvl` file back into the editor.

Saved data includes:
- rows
- cols
- spawn
- tiles
- traps
- enemies
- vines

In short, `LevelIO` is what makes editor data persist as level files.

---

## Full Process: How Everything Connects

A teammate should think of the pipeline like this:

1. `MapTile` defines what tile IDs exist.
2. `MapGrid` stores those tile IDs in a 2D grid, renders them, and handles collision.
3. `EditorUI` lets the user choose a brush or tool.
4. `LevelEditor` reads that brush choice and places tiles, enemies, traps, spawn, or vines into runtime editor state.
5. `LevelIO` converts that runtime editor state into level data and saves it to a `.lvl` file, or loads a `.lvl` file back into the editor.

---

## How to Add a New Tile Type

Adding a tile is a multi-file change. Do not change only one place.

### 1. Add the tile enum in `MapTile.h`
Add the new tile before `MAP_TILE_TYPE_COUNT`.

Example:
```cpp
enum Type
{
    NONE = 0,
    GROUND_SURFACE,
    GROUND_BODY,
    GROUND_BOTTOM,
    PLATFORM,
    ICE_TILE,

    MAP_TILE_TYPE_COUNT,
};