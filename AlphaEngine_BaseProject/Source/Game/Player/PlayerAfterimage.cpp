#include "PlayerAfterimage.h"

#include <AEEngine.h>

#include "Player.h"
#include "../Camera.h"
#include "../Time.h"
#include "../../Utils/MeshGenerator.h"
#include "../../Utils/AEExtras.h"

void PlayerAfterimage::Afterimage::Init()
{
}

void PlayerAfterimage::Afterimage::OnGet()
{
}

void PlayerAfterimage::Afterimage::OnRelease()
{
}

void PlayerAfterimage::Afterimage::Exit()
{
}

PlayerAfterimage::PlayerAfterimage(const Player& player, const AEVec2& size) :
	pool(10),
	refPlayer(player),
	lastSpawnedPos(player.GetPosition()),
	lifetime(0.2f)
{
	const Sprite& sprite = player.GetSprite();
	texture = AEGfxTextureLoad(sprite.GetFile());

	AEVec2 uvSize = sprite.GetUV_Size();
	mesh = MeshGenerator::GetRectMesh(size.x, size.y, uvSize.x, uvSize.y);

	// Selecting the frames. 
	// lhs/rhs
	// - lhs: frame/state number
	// - rhs: total frame/state
	uvOffsets[Afterimage::DASH_UP]			= { 1 / 8.f,  3 / 21.f };
	uvOffsets[Afterimage::DASH_DOWN]		= { 1 / 8.f, 15 / 21.f };
	uvOffsets[Afterimage::DASH_HORIZONTAL]	= { 3 / 8.f,  2 / 21.f };
	uvOffsets[Afterimage::SMASH_ATTACK]		= { 2 / 8.f, 19 / 21.f };

	spacings[Afterimage::DASH_UP] = 1.f;
	spacings[Afterimage::DASH_DOWN] = 1.f;
	spacings[Afterimage::DASH_HORIZONTAL] = 0.5f;
	spacings[Afterimage::SMASH_ATTACK] = 0.05f;
}

PlayerAfterimage::~PlayerAfterimage()
{
	AEGfxTextureUnload(texture);
	AEGfxMeshFree(mesh);
}

void PlayerAfterimage::Update()
{
	float currTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());
	for (int i = static_cast<int>(pool.GetSize()) - 1; i >= 0; --i)
	{
		Afterimage& img = pool.pool[i];
		if (currTime > img.spawnTime + lifetime)
			pool.Release(img);
	}

	if (!refPlayer.IsDashing() && refPlayer.GetAnimState() != Player::AnimState::AIR_ATTACK_SMASH)
		return;

	bool isDashing = refPlayer.IsDashing();

	Afterimage::Type type = Afterimage::Type::SMASH_ATTACK;
	if (isDashing)
	{
		const AEVec2& vel = refPlayer.GetVelocity();
		if (fabsf(vel.x) > fabsf(vel.y))
			type = Afterimage::Type::DASH_HORIZONTAL;
		else
			type = vel.y > 0 ? Afterimage::Type::DASH_UP : Afterimage::Type::DASH_DOWN;
	}

	const AEVec2& playerPos = refPlayer.GetPosition();
	AEVec2 displacement = playerPos - lastSpawnedPos;
	float sqrDist = AEExtras::SqrDist(displacement);
	float spacing = spacings[type];

	if (sqrDist > spacing * spacing)
	{
		float dist = sqrtf(sqrDist);
		AEVec2 direction = displacement / dist;

		while (dist > spacing)
		{
			lastSpawnedPos += direction * spacing;
			dist -= spacing;

			Afterimage& afterimg = pool.Get();
			afterimg.position = lastSpawnedPos;
			afterimg.isFacingRight = refPlayer.GetIsFacingRight();
			afterimg.spawnTime = currTime;
			afterimg.type = type;
		}
	}
}

void PlayerAfterimage::ResetLastSpawn()
{
	lastSpawnedPos = refPlayer.GetPosition();
}

void PlayerAfterimage::Render()
{
	AEMtx33 transform;
	float currTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());
	const AEVec2& spritePivot = refPlayer.GetSprite().metadata.pivot;

	for (size_t i = 0; i < pool.GetSize(); i++)
	{
		Afterimage& obj = pool.pool[i];

		if (obj.isFacingRight)
			AEMtx33Identity(&transform);
		else
			AEMtx33Scale(&transform, -1.f, 1.f);

		AEMtx33TransApply(
			&transform,
			&transform,
			obj.position.x - (0.5f - spritePivot.x),
			obj.position.y + (0.5f - spritePivot.y)
		);
		// Camera scale. 
		AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
		AEGfxSetTransform(transform.m);

		const AEVec2& uvOffset = uvOffsets[obj.type];
		AEGfxTextureSet(texture, uvOffset.x, uvOffset.y);

		float t = (currTime - obj.spawnTime) / lifetime;
		AEGfxSetTransparency((1.f - t) * 0.75f);

		AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	}

	AEGfxSetTransparency(1.f);
}
