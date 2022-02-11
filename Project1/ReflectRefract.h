#pragma once

#include "CubeMap.h"

enum class ReflectRefractType
{
	None,
	Reflect,
	Refract
};

enum class ReflectRefractMapType
{
	Environment,
	DynamicMinimal,
	DynamicMedium,
	DynamicFull,
	Custom
};


enum class ReflectRefractMapPriorityType
{
	High,
	Medium,
	Minimal
};

struct ReflectRefractData
{
	ReflectRefractData(ReflectRefractType type, ReflectRefractMapType mapType, CubeMap* customMap, float strength, float refractRatio)
		: type(type),
		mapType(mapType),
		customMap(customMap),
		strength(strength),
		refractRatio(refractRatio)
	{}

	ReflectRefractData() 
		: type(ReflectRefractType::None),
		mapType(ReflectRefractMapType::Environment),
		customMap(nullptr),
		strength(0.0f),
		refractRatio(0.0f)
	{}

	ReflectRefractType type;
	ReflectRefractMapType mapType;
	CubeMap* customMap;
	float strength;
	float refractRatio;
};