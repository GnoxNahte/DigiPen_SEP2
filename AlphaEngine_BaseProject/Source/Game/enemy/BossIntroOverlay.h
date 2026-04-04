/*!
@file   BossIntroOverlay.h
@author lim kang ping
@brief  This file declares the boss intro overlay functions.
It provides functions to start, reset, update, render,
and check the state of the boss intro screen.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once

namespace BossIntroOverlay
{
    void Init();
    void Exit();

    void Start();
    void Reset();

    void Update(float dt);
    void Render();

    bool IsActive();
}