#pragma once

namespace Physics
{
	enum class ShapeType
	{
		Box,
		Sphere,
		Plane,
		Cylinder,
		Capsule,
		Mesh,
		ScaledMesh
	};

	class IShape
	{
	public:
		virtual ~IShape() = default;

		ShapeType GetShapeType() const { return shapeType; }

	protected:
		IShape(ShapeType shapeType) : shapeType(shapeType) {}

	private:
		IShape(const IShape& other) {}
		IShape& operator=(const IShape& other) { return *this; }

		ShapeType shapeType;
	};
}