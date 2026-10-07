// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMeshComponent;

/**
 * Demo 的统一配色表。
 * 想统一调色就改这里，不用满项目找颜色字面量。
 */
namespace LastDayColors
{
	/** 建造物（采集器等白模）—— 黄色 */
	inline const FLinearColor Construction(1.00f, 0.80f, 0.05f, 1.0f);

	/** 金属件（炮台、武器、子弹）—— 偏灰的银色 */
	inline const FLinearColor Metal(0.45f, 0.46f, 0.48f, 1.0f);

	/** 源晶玻璃 —— 亮紫 */
	inline const FLinearColor Glass(0.72f, 0.22f, 1.00f, 1.0f);

	/** 敌人 —— 紫色 */
	inline const FLinearColor Enemy(0.55f, 0.08f, 1.00f, 1.0f);

	/** 怪物（新敌人）—— 红色 */
	inline const FLinearColor Monster(0.95f, 0.06f, 0.06f, 1.0f);
}

/**
 * 白模上色：把材质换成引擎自带、带 Color 参数的白模材质再上色。
 * （/Engine/BasicShapes 的 Cube / Sphere 默认材质是 WorldGridMaterial / DefaultMaterial，
 *   它们没有 Color 参数，直接建动态实例改颜色是空操作。）
 */
void ApplyLastDayColor(UMeshComponent* MeshComp, const FLinearColor& Color);

/** 金属材质（Metallic = 1，颜色走 Color 参数）：炮台、武器、子弹用 */
void ApplyLastDayMetalColor(UMeshComponent* MeshComp, const FLinearColor& Color);

/** 玻璃材质（半透明，低粗糙度）：源晶用 */
void ApplyLastDayGlassColor(UMeshComponent* MeshComp, const FLinearColor& Color);
