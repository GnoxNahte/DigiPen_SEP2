#include "Player.h"
#include "../../Utils/MeshGenerator.h"

Player::Player() : stats("player-stats.json"), spriteSheet("Assets/Craftpix/Char_Robot.png")
{
}

Player::~Player()
{
}

void Player::Update()
{
	spriteSheet.Update();
}

void Player::Render()
{
    AEMtx33 scale = { 0 };
    AEMtx33Scale(&scale, 10, 10);

    // Choose the transform to use
    AEGfxSetTransform(scale.m);
	spriteSheet.Render();
}
