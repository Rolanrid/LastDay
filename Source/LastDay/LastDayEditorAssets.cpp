// Copyright Epic Games, Inc. All Rights Reserved.

#include "LastDayEditorAssets.h"
#include "LastDay.h"

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	const TCHAR* ExplosionMaterialPackage = TEXT("/Game/FX/M_Explosion");
	const TCHAR* ExplosionMaterialAssetName = TEXT("M_Explosion");
	const TCHAR* ExplosionMaterialObjectPath = TEXT("/Game/FX/M_Explosion.M_Explosion");

	/**
	 * 生成爆炸材质：Translucent + Unlit，带 Color（向量）与 Opacity（标量）两个参数。
	 * 引擎自带的半透材质（粒子、3D Widget 等）透明度都由粒子色/顶点色驱动，静态网格上恒为 1，
	 * 没法从代码控制，所以这里给爆炸单独生成一个。
	 */
	void CreateExplosionMaterial()
	{
		UPackage* Package = CreatePackage(ExplosionMaterialPackage);
		if (!Package)
		{
			UE_LOG(LogLastDay, Warning, TEXT("[EditorAssets] 创建爆炸材质失败：无法创建包 %s"), ExplosionMaterialPackage);
			return;
		}

		UMaterial* Material = NewObject<UMaterial>(Package, ExplosionMaterialAssetName, RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;

		UMaterialExpressionVectorParameter* ColorParameter = NewObject<UMaterialExpressionVectorParameter>(Material);
		ColorParameter->ParameterName = TEXT("Color");
		ColorParameter->DefaultValue = FLinearColor::FromSRGBColor(FColor(255, 69, 0)); // 橙红
		Material->GetExpressionCollection().AddExpression(ColorParameter);

		UMaterialExpressionScalarParameter* OpacityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityParameter->ParameterName = TEXT("Opacity");
		OpacityParameter->DefaultValue = 1.0f;
		Material->GetExpressionCollection().AddExpression(OpacityParameter);

		if (UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData())
		{
			EditorData->EmissiveColor.Expression = ColorParameter;
			EditorData->Opacity.Expression = OpacityParameter;
		}

		Material->PostEditChange();
		Material->ForceRecompileForRendering();
		FAssetRegistryModule::AssetCreated(Material);
		Package->MarkPackageDirty();

		const FString FileName = FPackageName::LongPackageNameToFilename(ExplosionMaterialPackage, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		const bool bSaved = UPackage::SavePackage(Package, Material, *FileName, SaveArgs);

		UE_LOG(LogLastDay, Log, TEXT("[EditorAssets] 自动生成爆炸材质 %s：%s"),
			ExplosionMaterialObjectPath, bSaved ? TEXT("已保存") : TEXT("保存失败"));
	}
}

void EnsureLastDayEditorAssets()
{
	if (!GIsEditor)
	{
		return;
	}

	if (LoadObject<UMaterialInterface>(nullptr, ExplosionMaterialObjectPath))
	{
		return; // 已经有了
	}

	CreateExplosionMaterial();
}

// 也可以在编辑器控制台（Output Log 的命令输入框）手动执行，省得重启编辑器
static FAutoConsoleCommand CreateExplosionMaterialCommand(
	TEXT("LastDay.CreateExplosionMaterial"),
	TEXT("创建爆炸用的半透材质 /Game/FX/M_Explosion（已存在则跳过）"),
	FConsoleCommandDelegate::CreateStatic(&EnsureLastDayEditorAssets)
);

#else

void EnsureLastDayEditorAssets() {}

#endif // WITH_EDITOR
