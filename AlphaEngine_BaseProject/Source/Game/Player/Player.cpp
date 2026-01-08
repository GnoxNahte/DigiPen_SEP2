#include "Player.h"
#include "../Camera.h"

Player::Player() : stats("player-stats.json"), spriteSheet("Assets/Craftpix/Char_Robot.png")
{
}

Player::~Player()
{
}

void Player::Update()
{
	spriteSheet.Update();

    // @todo: replace with stats
    float tmpSpeed = 1;

    if (AEInputCheckCurr(AEVK_LEFT))
        position.x -= tmpSpeed;
    if (AEInputCheckCurr(AEVK_RIGHT))
        position.x += tmpSpeed;
}

void Player::Render()
{
    AEMtx33Trans(&transform, position.x, position.y);
    AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);

    AEGfxSetTransform(transform.m);
	spriteSheet.Render();
}
