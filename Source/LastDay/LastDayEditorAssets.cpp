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
	/** 自动生成材质的信息 */
	struct FGeneratedMaterial
	{
		const TCHAR* PackagePath;
		const TCHAR* AssetName;
		const TCHAR* ObjectPath;
	};

	// 爆炸：半透 + Unlit，带 Color / Opacity 两个参数
	const FGeneratedMaterial ExplosionMaterialInfo{ TEXT("/Game/FX/M_Explosion"), TEXT("M_Explosion"), TEXT("/Game/FX/M_Explosion.M_Explosion") };
	// 金属：不透明 + DefaultLit，Color（银色）+ Metallic + Roughness
	const FGeneratedMaterial MetalMaterialInfo{ TEXT("/Game/FX/M_Metal"), TEXT("M_Metal"), TEXT("/Game/FX/M_Metal.M_Metal") };
	// 玻璃：半透 + DefaultLit，Color（淡蓝）+ Opacity + Roughness + Specular
	const FGeneratedMaterial GlassMaterialInfo{ TEXT("/Game/FX/M_Glass"), TEXT("M_Glass"), TEXT("/Game/FX/M_Glass.M_Glass") };

	/** 加一个向量参数 */
	UMaterialExpressionVectorParameter* AddVectorParameter(UMaterial* Material, const TCHAR* ParameterName, const FLinearColor& DefaultValue)
	{
		UMaterialExpressionVectorParameter* Parameter = NewObject<UMaterialExpressionVectorParameter>(Material);
		Parameter->ParameterName = ParameterName;
		Parameter->DefaultValue = DefaultValue;
		Material->GetExpressionCollection().AddExpression(Parameter);
		return Parameter;
	}

	/** 加一个标量参数 */
	UMaterialExpressionScalarParameter* AddScalarParameter(UMaterial* Material, const TCHAR* ParameterName, float DefaultValue)
	{
		UMaterialExpressionScalarParameter* Parameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		Parameter->ParameterName = ParameterName;
		Parameter->DefaultValue = DefaultValue;
		Material->GetExpressionCollection().AddExpression(Parameter);
		return Parameter;
	}

	/** 生成材质的收尾：编译、注册、保存 */
	void FinishAndSaveMaterial(UPackage* Package, UMaterial* Material, const FGeneratedMaterial& Info)
	{
		Material->PostEditChange();
		Material->ForceRecompileForRendering();
		FAssetRegistryModule::AssetCreated(Material);
		Package->MarkPackageDirty();

		const FString FileName = FPackageName::LongPackageNameToFilename(Info.PackagePath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		const bool bSaved = UPackage::SavePackage(Package, Material, *FileName, SaveArgs);

		UE_LOG(LogLastDay, Log, TEXT("[EditorAssets] 自动生成材质 %s：%s"),
			Info.ObjectPath, bSaved ? TEXT("已保存") : TEXT("保存失败"));
	}

	/**
	 * 爆炸材质：Translucent + Unlit，带 Color（向量）与 Opacity（标量）两个参数。
	 * 引擎自带的半透材质透明度都由粒子色/顶点色驱动，静态网格上恒为 1，没法从代码控制。
	 */
	void CreateExplosionMaterial()
	{
		UPackage* Package = CreatePackage(ExplosionMaterialInfo.PackagePath);
		if (!Package)
		{
			UE_LOG(LogLastDay, Warning, TEXT("[EditorAssets] 创建爆炸材质失败：无法创建包 %s"), ExplosionMaterialInfo.PackagePath);
			return;
		}

		UMaterial* Material = NewObject<UMaterial>(Package, ExplosionMaterialInfo.AssetName, RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;

		UMaterialExpressionVectorParameter* ColorParameter = AddVectorParameter(Material, TEXT("Color"), FLinearColor::FromSRGBColor(FColor(255, 69, 0)));
		UMaterialExpressionScalarParameter* OpacityParameter = AddScalarParameter(Material, TEXT("Opacity"), 1.0f);

		if (UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData())
		{
			EditorData->EmissiveColor.Expression = ColorParameter;
			EditorData->Opacity.Expression = OpacityParameter;
		}

		FinishAndSaveMaterial(Package, Material, ExplosionMaterialInfo);
	}

	/** 金属材质：不透明 + DefaultLit，Metallic = 1，颜色走 Color 参数（默认银色） */
	void CreateMetalMaterial()
	{
		UPackage* Package = CreatePackage(MetalMaterialInfo.PackagePath);
		if (!Package)
		{
			UE_LOG(LogLastDay, Warning, TEXT("[EditorAssets] 创建金属材质失败：无法创建包 %s"), MetalMaterialInfo.PackagePath);
			return;
		}

		UMaterial* Material = NewObject<UMaterial>(Package, MetalMaterialInfo.AssetName, RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_DefaultLit);

		UMaterialExpressionVectorParameter* ColorParameter = AddVectorParameter(Material, TEXT("Color"), FLinearColor(0.85f, 0.86f, 0.90f, 1.0f));
		UMaterialExpressionScalarParameter* MetallicParameter = AddScalarParameter(Material, TEXT("Metallic"), 1.0f);
		UMaterialExpressionScalarParameter* RoughnessParameter = AddScalarParameter(Material, TEXT("Roughness"), 0.28f);

		if (UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData())
		{
			EditorData->BaseColor.Expression = ColorParameter;
			EditorData->Metallic.Expression = MetallicParameter;
			EditorData->Roughness.Expression = RoughnessParameter;
		}

		FinishAndSaveMaterial(Package, Material, MetalMaterialInfo);
	}

	/** 玻璃材质：半透 + DefaultLit，低粗糙度、高高光，颜色和透明度都走参数 */
	void CreateGlassMaterial()
	{
		UPackage* Package = CreatePackage(GlassMaterialInfo.PackagePath);
		if (!Package)
		{
			UE_LOG(LogLastDay, Warning, TEXT("[EditorAssets] 创建玻璃材质失败：无法创建包 %s"), GlassMaterialInfo.PackagePath);
			return;
		}

		UMaterial* Material = NewObject<UMaterial>(Package, GlassMaterialInfo.AssetName, RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;

		UMaterialExpressionVectorParameter* ColorParameter = AddVectorParameter(Material, TEXT("Color"), FLinearColor(0.45f, 0.70f, 1.0f, 1.0f));
		UMaterialExpressionScalarParameter* OpacityParameter = AddScalarParameter(Material, TEXT("Opacity"), 0.35f);
		UMaterialExpressionScalarParameter* RoughnessParameter = AddScalarParameter(Material, TEXT("Roughness"), 0.06f);
		UMaterialExpressionScalarParameter* SpecularParameter = AddScalarParameter(Material, TEXT("Specular"), 1.0f);

		if (UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData())
		{
			EditorData->BaseColor.Expression = ColorParameter;
			EditorData->Opacity.Expression = OpacityParameter;
			EditorData->Roughness.Expression = RoughnessParameter;
			EditorData->Specular.Expression = SpecularParameter;
		}

		FinishAndSaveMaterial(Package, Material, GlassMaterialInfo);
	}
}

void EnsureLastDayEditorAssets()
{
	if (!GIsEditor)
	{
		return;
	}

	if (!LoadObject<UMaterialInterface>(nullptr, ExplosionMaterialInfo.ObjectPath))
	{
		CreateExplosionMaterial();
	}

	if (!LoadObject<UMaterialInterface>(nullptr, MetalMaterialInfo.ObjectPath))
	{
		CreateMetalMaterial();
	}

	if (!LoadObject<UMaterialInterface>(nullptr, GlassMaterialInfo.ObjectPath))
	{
		CreateGlassMaterial();
	}
}

// 也可以在编辑器控制台（Output Log 的命令输入框）手动执行，省得重启编辑器
static FAutoConsoleCommand CreateLastDayMaterialsCommand(
	TEXT("LastDay.CreateMaterials"),
	TEXT("创建 Demo 需要的自动生成材质：爆炸 /Game/FX/M_Explosion、金属 /Game/FX/M_Metal、玻璃 /Game/FX/M_Glass（已存在则跳过）"),
	FConsoleCommandDelegate::CreateStatic(&EnsureLastDayEditorAssets)
);

#else

void EnsureLastDayEditorAssets() {}

#endif // WITH_EDITOR
