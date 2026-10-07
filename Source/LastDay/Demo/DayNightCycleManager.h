// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DayNightCycleManager.generated.h"

class ADirectionalLight;
class ASkyAtmosphere;
class ASkyLight;
class AVolumetricCloud;

/**
 * 连续的昼夜天色。
 *
 * 时间轴自己一直往前跑：太阳的高度角、方位角、颜色、强度一路插值出来，天色是连续变化的，
 * 不会在白天/黄昏/夜晚之间跳变。
 *
 * 夜里用一盏单独的"月亮"平行光照场景：它不参与天空大气（bAtmosphereSunLight = false），
 * 所以天上不会出现太阳；而真正的太阳在天黑后强度降到 0、并且落到地平线以下，
 * 天上自然也看不到它。
 */
UCLASS()
class LASTDAY_API ADayNightCycleManager : public AActor
{
	GENERATED_BODY()

public:
	ADayNightCycleManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------------- 时间轴 ----------------

	/** 一个完整昼夜循环的时长（秒） */
	UPROPERTY(EditAnywhere, Category = "Lighting|Cycle", meta = (ClampMin = "1.0"))
	float DayLengthSeconds = 240.0f;

	/** 起始时刻：0=午夜，0.25=日出，0.5=正午，0.75=日落 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Cycle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StartTimeOfDay = 0.3f;

	/** 当前时刻（0~1，只读）：0.5 是正午 */
	UPROPERTY(BlueprintReadOnly, Category = "Lighting|Cycle")
	float TimeOfDay = 0.0f;

	/** 太阳高度角在 ±这个范围（度）内完成白天与夜晚的过渡，越大晨昏越长越柔和 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Cycle", meta = (ClampMin = "1.0"))
	float TwilightRange = 26.0f;

	// ---------------- 太阳 ----------------

	/** 白天太阳强度 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Sun", meta = (ClampMin = "0.0"))
	float DaySunIntensity = 6.0f;

	/** 白天太阳颜色 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Sun")
	FLinearColor DaySunColor = FLinearColor(1.0f, 0.97f, 0.90f, 1.0f);

	/** 日出 / 日落时太阳的颜色（地平线附近偏橙红） */
	UPROPERTY(EditAnywhere, Category = "Lighting|Sun")
	FLinearColor HorizonSunColor = FLinearColor(1.0f, 0.42f, 0.12f, 1.0f);

	/** 夜里给天空和云留的一点亮度（太阳灯强度的下限）：太小晚上云看不见，太大天会发亮 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Sun", meta = (ClampMin = "0.0"))
	float NightSunIntensity = 0.01f;

	// ---------------- 月亮（夜里照亮场景，但不会在天上画太阳）----------------

	/** 夜晚月光强度：这就是夜里亮不亮的主要旋钮 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Moon", meta = (ClampMin = "0.0"))
	float NightMoonIntensity = 0.038f;

	/** 夜晚月光颜色（深蓝紫） */
	UPROPERTY(EditAnywhere, Category = "Lighting|Moon")
	FLinearColor NightMoonColor = FLinearColor(0.42f, 0.16f, 0.88f, 1.0f);

	/**
	 * 月亮是否参与天空大气（作为第二颗大气光源）。
	 * 开着：夜里的天空和云会被月光微微照亮，天空不至于一片漆黑（代价是天上会出现一个小小的月亮圆盘）；
	 * 关掉：夜空会很暗，但天上也不会有任何圆盘。
	 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Moon")
	bool bMoonLightsSky = true;

	// ---------------- 天空光（环境光）----------------

	/** 白天天空光强度 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Ambient", meta = (ClampMin = "0.0"))
	float DaySkyIntensity = 1.8f;

	/** 夜晚天空光强度：夜里靠它兜底，太小会全黑、太大会发灰 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Ambient", meta = (ClampMin = "0.0"))
	float NightSkyIntensity = 0.35f;

	/** 白天天空光颜色 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Ambient")
	FLinearColor DaySkyColor = FLinearColor::White;

	/** 夜晚天空光颜色（深蓝紫色滤镜） */
	UPROPERTY(EditAnywhere, Category = "Lighting|Ambient")
	FLinearColor NightSkyColor = FLinearColor(0.38f, 0.18f, 0.92f, 1.0f);

	/** 夜晚下半球（地面、背光面）的补光颜色，避免阴影里死黑 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Ambient")
	FLinearColor NightGroundColor = FLinearColor(0.08f, 0.04f, 0.16f, 1.0f);

	// ---------------- 云 ----------------

	/**
	 * 云量（对应云材质的 Cloud_GlobalCoverage 参数）。
	 * 注意这是"偏置"而不是百分比：引擎自带实例的默认值是 -0.2，
	 * 往负方向调云更少、往正方向调云更多。
	 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Cloud", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float CloudCoverage = 0.0f;

	// ---------------- 影子 ----------------

	/** 光源角度（度）：越大影子边缘越柔和、整体越淡 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Shadows", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float LightSourceAngle = 3.0f;

	/**
	 * 风（对应云材质的 Layout_WindControls 参数，是让云动起来的唯一有效旋钮）：
	 * XYZ = 沿世界 X/Y/Z 轴的风速（可正可负），A = 统一放大/缩小风速。
	 * 引擎自带实例的默认值只有 (1, 1, 0.5, 0.333)，所以默认状态下云几乎不动。
	 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Cloud")
	FLinearColor CloudWindControls = FLinearColor(8.0f, 6.0f, 0.5f, 1.0f);

	/** 白天的云颜色（写进云材质的 Cloud_AlbedoColor 参数） */
	UPROPERTY(EditAnywhere, Category = "Lighting|Cloud")
	FLinearColor DayCloudColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	/** 夜晚的云颜色：压暗偏紫，避免夜里云白得发亮 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Cloud")
	FLinearColor NightCloudColor = FLinearColor(0.22f, 0.14f, 0.42f, 1.0f);

	// ---------------- 场景准备 ----------------

	/** 关卡里缺 SkyLight / SkyAtmosphere 时是否自动补一个 */
	UPROPERTY(EditAnywhere, Category = "Lighting|Setup")
	bool bEnsureMissingSkyActors = true;

	/** 天光是否用实时捕获（天色一直在变，实时捕获才能跟着变） */
	UPROPERTY(EditAnywhere, Category = "Lighting|Setup")
	bool bUseRealTimeSkyCapture = true;

	/** 屏幕上显示当前时刻、太阳高度角与各光源强度（调试用） */
	UPROPERTY(EditAnywhere, Category = "Lighting|Debug")
	bool bShowDebugInfo = false;

protected:
	/** 找到或补上 DirectionalLight（太阳 / 月亮）/ SkyLight / SkyAtmosphere，并保证运行时能改动它们 */
	void EnsureSkyActors();

	/** 按当前时刻更新太阳、月亮与天空光 */
	void UpdateSky(float DeltaSeconds);

	/** 太阳：参与天空大气，天上能看到的那颗 */
	UPROPERTY() TObjectPtr<ADirectionalLight> SunLight;

	/** 月亮：夜里照亮场景，但不参与大气，所以不会在天上画出太阳 */
	UPROPERTY() TObjectPtr<ADirectionalLight> MoonLight;

	/** 天空光 */
	UPROPERTY() TObjectPtr<ASkyLight> SkyLight;

	/** 天空大气：有它才能看到随昼夜变化的天空颜色 */
	UPROPERTY() TObjectPtr<ASkyAtmosphere> SkyAtmosphere;

	/** 体积云（关卡里没有就补一个） */
	UPROPERTY() TObjectPtr<AVolumetricCloud> CloudActor;

	/** 云的动态材质实例（用来按昼夜改云的颜色） */
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CloudMaterialInstance;

	/** 上一次实际写进天光的下半球颜色（避免每帧都 dirty 渲染状态） */
	FLinearColor CachedGroundColor = FLinearColor(-1.0f, -1.0f, -1.0f, -1.0f);

	/** 上一次实际写进云的云颜色（同上，避免每帧都改材质） */
	FLinearColor CachedCloudColor = FLinearColor(-1.0f, -1.0f, -1.0f, -1.0f);
};
