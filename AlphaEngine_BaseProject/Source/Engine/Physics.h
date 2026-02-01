#pragma once
#include <AEEngine.h>

enum ShapeType
{
	Point,
	Circle,
	Rect,
	//Capsule,
};

class BaseCollider
{
public:
	virtual ShapeType GetType() const = 0;
};

class CircleCollider : public BaseCollider
{
public:
	float radius;
	ShapeType GetType() const override;
};

class RectCollider : public BaseCollider
{
public:
	AEVec2 size;
	ShapeType GetType() const override;
};

// No members. Same as BaseCollider except it returns Point for GetType.
// @todo - delete if not needed?
class PointColldier : public BaseCollider
{
public:
	ShapeType GetType() const override;
};
