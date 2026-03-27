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
	// Selecting the frames. 
	// lhs/rhs
	// - lhs: frame/state number
	// - rhs: total frame/state
	dash_uvOffset(3.f/8.f, 2.f/21.f), 
	slam_uvOffset(2.f/8.f, 19.f/21.f),
	lifetime(0.5f),
	dashSpacing(0.5f),
	slamSpacing(1.f)
{
	const Sprite& sprite = player.GetSprite();
	texture = AEGfxTextureLoad(sprite.GetFile());

	AEVec2 uvSize = sprite.GetUV_Size();
	mesh = MeshGenerator::GetRectMesh(size.x, size.y, uvSize.x, uvSize.y);
}

PlayerAfterimage::~PlayerAfterimage()
{
	AEGfxTextureUnload(texture);
	AEGfxMeshFree(mesh);
}

void PlayerAfterimage::Update()
{
	if (!refPlayer.IsDashing() && refPlayer.GetAnimState() != Player::AnimState::AIR_ATTACK_SMASH)
		return;

	float currTime = static_cast<float>(Time::GetInstance().GetScaledElapsedTime());
	if (AEInputCheckTriggered(AEVK_K))
	{
		Afterimage& afterimg = pool.Get();
		afterimg.position = refPlayer.GetPosition();
		afterimg.isFacingRight = refPlayer.GetIsFacingRight();
		afterimg.spawnTime = currTime;
	}

	for (int i = static_cast<int>(pool.GetSize()) - 1; i >= 0; --i)
	{
		Afterimage& img = pool.pool[i];
		if (currTime > img.spawnTime + lifetime)
			pool.Release(img);
	}

	bool isDashing = refPlayer.IsDashing();

	const AEVec2& playerPos = refPlayer.GetPosition();
	AEVec2 displacement = playerPos - lastSpawnedPos;
	float sqrDist = AEExtras::SqrDist(displacement);
	float spacing = isDashing ? dashSpacing : slamSpacing;

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
			afterimg.isDashing = isDashing;
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

		AEMtx33Trans(
			&transform,
			obj.position.x - (0.5f - spritePivot.x),
			obj.position.y + (0.5f - spritePivot.y)
		);
		// Camera scale. 
		AEMtx33ScaleApply(&transform, &transform, Camera::scale, Camera::scale);
		AEGfxSetTransform(transform.m);

		if (obj.isDashing)
			AEGfxTextureSet(texture, dash_uvOffset.x, dash_uvOffset.y);
		else
			AEGfxTextureSet(texture, slam_uvOffset.x, slam_uvOffset.y);

		AEGfxSetTransparency(1.f - (currTime - obj.spawnTime) / lifetime);

		AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	}

	AEGfxSetTransparency(1.f);
}
