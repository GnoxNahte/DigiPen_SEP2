#pragma once
#include <AEEngine.h>

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