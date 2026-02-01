#include "Physics.h"
#include "CommonTypes.h"

ShapeType CircleCollider::GetType() const
{
    return ShapeType::Circle;
}

ShapeType RectCollider::GetType() const
{
    return ShapeType::Rect;
}

ShapeType PointColldier::GetType() const
{
    return ShapeType::Point;
}
