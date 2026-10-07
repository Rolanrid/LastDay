// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DemoTypes.generated.h"

/** Demo 的昼夜/波次阶段（对应 H1-1） */
UENUM(BlueprintType)
enum class EDayPhase : uint8
{
	Landing  UMETA(DisplayName = "着陆"),
	Day      UMETA(DisplayName = "白天"),
	Dusk     UMETA(DisplayName = "黄昏"),
	Night    UMETA(DisplayName = "夜晚"),
	PostWave UMETA(DisplayName = "战后")
};
