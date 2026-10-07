// Copyright Epic Games, Inc. All Rights Reserved.

#include "LastDayVisuals.h"
#include "Components/MeshComponent.h"
#include "LastDay.h"
#include "LastDayEditorAssets.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	/** 引擎自带的白模材质：带 Color 向量参数，专门给 /Engine/BasicShapes 的白模上色 */
	const TCHAR* WhiteBoxMaterialPath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
	/** 项目自动生成的金属 / 玻璃材质（见 LastDayEditorAssets.cpp） */
	const TCHAR* MetalMaterialPath = TEXT("/Game/FX/M_Metal.M_Metal");
	const TCHAR* GlassMaterialPath = TEXT("/Game/FX/M_Glass.M_Glass");

	/** 只打一次材质状态日志，方便排查"颜色/材质没生效" */
	bool bLoggedMaterials = false;

	// 金属：偏灰、反射调低（Metallic 1 太像镜子）
	constexpr float MetalMetallic = 0.3f;
	constexpr float MetalRoughness = 0.65f;

	// 玻璃：透明度调低 = 更实一点（Opacity 越大越不透明）
	constexpr float GlassOpacity = 0.7f;

	/** 加载材质；没有就请编辑器补生成一份再试（非编辑器构建里补生成是空操作） */
	UMaterialInterface* LoadMaterial(const TCHAR* ObjectPath)
	{
		if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, ObjectPath))
		{
			return Material;
		}

		EnsureLastDayEditorAssets();
		return LoadObject<UMaterialInterface>(nullptr, ObjectPath);
	}

	/** 换材质 → 建动态材质实例 → 设 Color；返回动态材质，方便继续设别的参数 */
	UMaterialInstanceDynamic* ApplyMaterialAndColor(UMeshComponent* MeshComp, UMaterialInterface* Material, const FLinearColor& Color)
	{
		if (Material)
		{
			MeshComp->SetMaterial(0, Material);
		}

		UMaterialInstanceDynamic* DynamicMaterial = MeshComp->CreateAndSetMaterialInstanceDynamic(0);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		}
		return DynamicMaterial;
	}

	void LogMaterialsOnce()
	{
		if (bLoggedMaterials)
		{
			return;
		}
		bLoggedMaterials = true;

		UE_LOG(LogLastDay, Log, TEXT("[Visuals] 材质：白模=%s  金属=%s  玻璃=%s"),
			LoadMaterial(WhiteBoxMaterialPath) ? TEXT("OK") : TEXT("缺失"),
			LoadMaterial(MetalMaterialPath) ? TEXT("OK") : TEXT("缺失"),
			LoadMaterial(GlassMaterialPath) ? TEXT("OK") : TEXT("缺失"));
	}
}

void ApplyLastDayColor(UMeshComponent* MeshComp, const FLinearColor& Color)
{
	if (!MeshComp)
	{
		return;
	}

	ApplyMaterialAndColor(MeshComp, LoadMaterial(WhiteBoxMaterialPath), Color);
	LogMaterialsOnce();
}

void ApplyLastDayMetalColor(UMeshComponent* MeshComp, const FLinearColor& Color)
{
	if (!MeshComp)
	{
		return;
	}

	// 金属材质缺失时（例如打包构建里没生成过它）退回白模上色，至少颜色是对的
	if (UMaterialInterface* MetalMaterial = LoadMaterial(MetalMaterialPath))
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = ApplyMaterialAndColor(MeshComp, MetalMaterial, Color))
		{
			// 反射调低：降低金属度、提高粗糙度
			DynamicMaterial->SetScalarParameterValue(TEXT("Metallic"), MetalMetallic);
			DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), MetalRoughness);
		}
	}
	else
	{
		ApplyMaterialAndColor(MeshComp, LoadMaterial(WhiteBoxMaterialPath), Color);
	}

	LogMaterialsOnce();
}

void ApplyLastDayGlassColor(UMeshComponent* MeshComp, const FLinearColor& Color)
{
	if (!MeshComp)
	{
		return;
	}

	if (UMaterialInterface* GlassMaterial = LoadMaterial(GlassMaterialPath))
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = ApplyMaterialAndColor(MeshComp, GlassMaterial, Color))
		{
			// 透明度调低（更实）：Opacity 越大越不透明
			DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), GlassOpacity);
		}
	}
	else
	{
		ApplyMaterialAndColor(MeshComp, LoadMaterial(WhiteBoxMaterialPath), Color);
	}

	LogMaterialsOnce();
}
