#pragma once
#include <limits>

namespace Easing
{
	// Reference: 
	// - https://github.com/Unity-Technologies/UnityCsReference/blob/59b03b8a0f179c0b7e038178c90b6c80b340aa9f/Runtime/Export/Math/Mathf.cs#L309
	// - https://theswissbay.ch/pdf/Gentoomen%20Library/Game%20Development/Programming/Game%20Programming%20Gems%204.pdf#page=111
	float SmoothDamp(float current, float target, float& currentSpeed, float smoothTime, float deltaTime, float maxSpeed = std::numeric_limits<float>::infinity())
	{
		float omega = 2.f / smoothTime;
		float x = omega * deltaTime;
		float exp = 1.f / (1.f + x + 0.48f * x * x + 0.235f * x * x * x);
		float change = AEClamp(current - target, -maxSpeed, maxSpeed);
		float tmp = (currentSpeed + omega * change) * deltaTime;
		currentSpeed = (currentSpeed - omega * tmp) * exp;
		return target + (change + tmp) * exp;
	}

    // Reference:
    // - https://github.com/Unity-Technologies/UnityCsReference/blob/59b03b8a0f179c0b7e038178c90b6c80b340aa9f/Runtime/Export/Math/Vector2.cs#L289
    static AEVec2 SmoothDamp(AEVec2 current, AEVec2 target, AEVec2& currentVelocity, float smoothTime, float deltaTime, float maxSpeed = std::numeric_limits<float>::infinity())
    {
        float omega = 2.f / smoothTime;
        float x = omega * deltaTime;
        float exp = 1.f / (1.f + x + 0.48f * x * x + 0.235f * x * x * x);

        AEVec2 change;
        AEVec2Sub(&change, &current, &target);
        
        // Clamp maximum velocity
        float maxChange = maxSpeed * smoothTime;
        float maxChangeSq = maxChange * maxChange;
        float sqDist = AEVec2SquareLength(&change);
        if (sqDist > maxChangeSq)
        {
            float multiplier = maxChange / sqrtf(sqDist);
            AEVec2Scale(&change, &change, multiplier);
        }

        AEVec2Sub(&target, &current, &change);
        AEVec2 temp(
            (currentVelocity.x + omega * change.x) * deltaTime,
            (currentVelocity.y + omega * change.y) * deltaTime
        );

        currentVelocity.x = (currentVelocity.x - omega * temp.x) * exp;
        currentVelocity.y = (currentVelocity.y - omega * temp.y) * exp;

        return AEVec2(target.x + (change.x + temp.x) * exp, target.y + (change.y + temp.y) * exp);
    }
}