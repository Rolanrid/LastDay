// Copyright Epic Games, Inc. All Rights Reserved.

#include "DayNightCycleManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"
#include "LastDay.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/**
	 * 运行时要把光源转起来、改强度，光照组件必须是 Movable。
	 * Static / Stationary 的光是烘焙的，运行时改不动 —— 这就是"太阳永远一个方向"的原因。
	 */
	void EnsureMovable(AActor* LightActor, const TCHAR* Label)
	{
		if (!LightActor)
		{
			return;
		}

		USceneComponent* Root = LightActor->GetRootComponent();
		if (Root && Root->Mobility != EComponentMobility::Movable)
		{
			Root->SetMobility(EComponentMobility::Movable);
			UE_LOG(LogLastDay, Warning, TEXT("[DayNight] %s 原本不是 Movable，已切到 Movable（建议直接在编辑器里改成 Movable）。"), Label);
		}
	}
}

ADayNightCycleManager::ADayNightCycleManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADayNightCycleManager::BeginPlay()
{
	Super::BeginPlay();

	TimeOfDay = FMath::Frac(FMath::Max(StartTimeOfDay, 0.0f));

	EnsureSkyActors();
	UpdateSky(0.0f);   // 开局先按当前时刻摆一次，不要等第一帧
}

void ADayNightCycleManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSky(DeltaSeconds);
}

void ADayNightCycleManager::EnsureSkyActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 关卡里已有的就直接用
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		SunLight = *It;
		break;
	}
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		SkyLight = *It;
		break;
	}
	for (TActorIterator<ASkyAtmosphere> It(World); It; ++It)
	{
		SkyAtmosphere = *It;
		break;
	}
	for (TActorIterator<AVolumetricCloud> It(World); It; ++It)
	{
		CloudActor = *It;
		break;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 太阳：连平行光都没有就补一个（角度下面每帧都会按时刻摆）
	if (!SunLight)
	{
		SunLight = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FTransform(FRotator(-45.0f, -60.0f, 0.0f)), SpawnParams);
	}

	// 月亮：单独一盏。它不参与天空大气，所以夜里照亮场景时天上不会冒出太阳
	if (!MoonLight)
	{
		MoonLight = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FTransform(FRotator(-45.0f, 120.0f, 0.0f)), SpawnParams);
	}

	if (bEnsureMissingSkyActors)
	{
		if (!SkyAtmosphere)
		{
			SkyAtmosphere = World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FTransform::Identity, SpawnParams);
		}

		if (!SkyLight)
		{
			SkyLight = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FTransform::Identity, SpawnParams);
		}

		if (!CloudActor)
		{
			// 引擎自带的 AVolumetricCloud 默认就带 m_SimpleVolumetricCloud_Inst 材质
			CloudActor = World->SpawnActor<AVolumetricCloud>(AVolumetricCloud::StaticClass(), FTransform::Identity, SpawnParams);
		}
	}

	EnsureMovable(SunLight, TEXT("DirectionalLight（太阳）"));
	EnsureMovable(MoonLight, TEXT("DirectionalLight（月亮）"));
	EnsureMovable(SkyLight, TEXT("SkyLight"));

	// 只有太阳参与大气：它是天上看到的那颗，也是日出日落天空颜色的来源
	if (SunLight)
	{
		if (UDirectionalLightComponent* Sun = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent()))
		{
			Sun->SetAtmosphereSunLight(true);
			// 不要太阳圆盘的外观（天空散射照旧）
			Sun->SetAtmosphereSunDiskColorScale(FLinearColor::Black);
			// 影子：白天也照，但要淡（光源角度越大边缘越柔和）
			Sun->SetCastShadows(true);
			Sun->SetLightSourceAngle(LightSourceAngle);
		}
	}

	if (MoonLight)
	{
		if (UDirectionalLightComponent* Moon = Cast<UDirectionalLightComponent>(MoonLight->GetLightComponent()))
		{
			// 月亮同样不要圆盘外观
			Moon->SetAtmosphereSunDiskColorScale(FLinearColor::Black);
			// 夜里的物体也要有影子，但同样调淡
			Moon->SetCastShadows(true);
			Moon->SetLightSourceAngle(LightSourceAngle);

			if (bMoonLightsSky)
			{
				// 把月亮当成"第二颗大气光源"（0 = 太阳，1 = 月亮）：
				// 只有参与大气，它才会照亮天空和体积云，夜里天空才不会一片漆黑
				Moon->SetAtmosphereSunLight(true);
				Moon->SetAtmosphereSunLightIndex(1);
			}
			else
			{
				Moon->SetAtmosphereSunLight(false);
			}
		}
	}

	if (SkyLight)
	{
		if (USkyLightComponent* Sky = Cast<USkyLightComponent>(SkyLight->GetLightComponent()))
		{
			if (bUseRealTimeSkyCapture)
			{
				// 天色一直在动，天光用实时捕获才能跟着变
				Sky->SetRealTimeCapture(true);
			}
			else
			{
				Sky->RecaptureSky();
			}
		}
	}

	// 云量：给云材质做一个动态实例，把总体覆盖率拧上去
	if (CloudActor)
	{
		if (UVolumetricCloudComponent* Cloud = CloudActor->FindComponentByClass<UVolumetricCloudComponent>())
		{
			if (UMaterialInterface* BaseMaterial = Cloud->Material.LoadSynchronous())
			{
				if (UMaterialInstanceDynamic* CloudMID = UMaterialInstanceDynamic::Create(BaseMaterial, Cloud))
				{
					// 参数名以引擎材质实际暴露的为准（用编辑器 Python 的 MaterialEditingLibrary 查过）：
					//   Cloud_GlobalCoverage —— 云量偏置（实例默认 -0.2，负值更少）
					//   Layout_WindControls  —— 风：XYZ 各轴风速，A 统一倍数（实例默认 1, 1, 0.5, 0.333，很慢）
					CloudMID->SetScalarParameterValue(TEXT("Cloud_GlobalCoverage"), FMath::Clamp(CloudCoverage, -1.0f, 1.0f));
					CloudMID->SetVectorParameterValue(TEXT("Layout_WindControls"), CloudWindControls);
					Cloud->SetMaterial(CloudMID);
					CloudMaterialInstance = CloudMID;

					UE_LOG(LogLastDay, Log, TEXT("[DayNight] 云：云量 %.2f，风 (%.2f, %.2f, %.2f, %.2f)"),
						CloudCoverage, CloudWindControls.R, CloudWindControls.G, CloudWindControls.B, CloudWindControls.A);
				}
			}
		}
	}
}

void ADayNightCycleManager::UpdateSky(float DeltaSeconds)
{
	// 1. 时间轴一直往前跑：天色连续变化，不按游戏阶段跳变
	if (DeltaSeconds > 0.0f)
	{
		TimeOfDay = FMath::Fmod(TimeOfDay + DeltaSeconds / FMath::Max(DayLengthSeconds, 1.0f), 1.0f);
		if (TimeOfDay < 0.0f)
		{
			TimeOfDay += 1.0f;
		}
	}

	// 2. 由时刻算出太阳的高度角与方位角
	const float SunAngle = 2.0f * PI * (TimeOfDay - 0.25f);   // 0.25 日出、0.5 正午、0.75 日落
	const float Elevation = 90.0f * FMath::Sin(SunAngle);      // 高度角（度）：负数表示落到地平线以下
	const float Azimuth = TimeOfDay * 360.0f - 90.0f;          // 方位角：一整天转一圈

	// 3. 白天权重：跨过地平线时平滑过渡（TwilightRange 越大，晨昏越长越柔和）
	const float Daylight = FMath::Clamp((Elevation + TwilightRange * 0.5f) / TwilightRange, 0.0f, 1.0f);
	const float NightWeight = 1.0f - Daylight;
	// 越接近地平线，日出 / 日落的橙红越明显
	const float HorizonGlow = FMath::Clamp(1.0f - FMath::Abs(Elevation) / (TwilightRange * 1.5f), 0.0f, 1.0f);

	// 4. 两个方向：太阳用真实高度角（夜里在地平线以下），月亮取反方向（太阳下山它就升起来）
	const float ElevRad = FMath::DegreesToRadians(Elevation);
	const float AzimRad = FMath::DegreesToRadians(Azimuth);
	const FVector ToSun(
		FMath::Cos(ElevRad) * FMath::Cos(AzimRad),
		FMath::Cos(ElevRad) * FMath::Sin(AzimRad),
		FMath::Sin(ElevRad));
	const FVector ToMoon = -ToSun;

	// 5. 各自的光照参数
	// 太阳：天黑后只留一点点，让天空和云还有一点微光（云刚好能看见）；
	// 因为太阳此时在地平线以下，天上不会挂着太阳
	const float SunIntensity = FMath::Lerp(NightSunIntensity, DaySunIntensity, Daylight);
	const FLinearColor SunColor = FMath::Lerp(DaySunColor, HorizonSunColor, HorizonGlow * 0.85f);
	// 月亮：天黑后慢慢亮起来，照亮场景但不画到天上
	const float MoonIntensity = NightMoonIntensity * NightWeight;

	const float SkyIntensity = FMath::Lerp(NightSkyIntensity, DaySkyIntensity, Daylight);
	const FLinearColor SkyColor = FMath::Lerp(NightSkyColor, DaySkyColor, Daylight);

	// 云的颜色也跟昼夜走：白天是白的，夜里压暗偏紫（夜里云太白就是反照率一直白）
	if (CloudMaterialInstance)
	{
		const FLinearColor CloudAlbedo = FMath::Lerp(NightCloudColor, DayCloudColor, Daylight);
		if (!CloudAlbedo.Equals(CachedCloudColor, 0.01f))
		{
			CachedCloudColor = CloudAlbedo;
			CloudMaterialInstance->SetVectorParameterValue(TEXT("Cloud_AlbedoColor"), CloudAlbedo);
		}
	}

	// 6. 应用到光源
	if (SunLight)
	{
		// 光线方向 = 从太阳指向场景
		SunLight->SetActorRotation((-ToSun).Rotation());

		if (UDirectionalLightComponent* Sun = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent()))
		{
			Sun->SetIntensity(SunIntensity);
			Sun->SetLightColor(SunColor);
		}
	}

	if (MoonLight)
	{
		MoonLight->SetActorRotation((-ToMoon).Rotation());

		if (UDirectionalLightComponent* Moon = Cast<UDirectionalLightComponent>(MoonLight->GetLightComponent()))
		{
			Moon->SetIntensity(MoonIntensity);
			Moon->SetLightColor(NightMoonColor);
		}
	}

	if (SkyLight)
	{
		if (USkyLightComponent* Sky = Cast<USkyLightComponent>(SkyLight->GetLightComponent()))
		{
			Sky->SetIntensity(SkyIntensity);
			Sky->SetLightColor(SkyColor);

			// 下半球补光：夜里给地面和背光面一点蓝紫，避免阴影死黑
			const FLinearColor GroundColor = FMath::Lerp(NightGroundColor, SkyColor, Daylight);
			if (!GroundColor.Equals(CachedGroundColor, 0.01f))
			{
				CachedGroundColor = GroundColor;
				Sky->SetLowerHemisphereColor(GroundColor);
			}
		}
	}

	if (bShowDebugInfo && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1201, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("时刻 %.2f（%s）  太阳高度角 %.1f°  太阳 %.2f  月光 %.2f  天光 %.2f"),
				TimeOfDay,
				(Elevation >= 0.0f ? TEXT("白天") : TEXT("夜晚")),
				Elevation,
				SunIntensity,
				MoonIntensity,
				SkyIntensity));
	}
}
