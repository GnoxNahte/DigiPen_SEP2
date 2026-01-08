#pragma once

#include <string>
#include "AEEngine.h"
#include "PlayerStats.h"
#include "../../Utils/Sprite.h"

/**
 * @brief Controllable player class
 */
class Player
{
public:
    AEVec2 position;

    Player();
    ~Player();
    void Update();
    void Render();
private:
    PlayerStats stats;
    Sprite spriteSheet;

    AEMtx33 transform;
};

