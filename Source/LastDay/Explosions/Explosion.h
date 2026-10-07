// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Explosion.generated.h"

/**
 * 爆炸基类。
 *
 * 暂时不做任何实现，只用来做继承判断（例如 Actor->IsA<AExplosion>()）。
 * 爆炸表现（AExplosionEffect）继承自它。
 */
UCLASS(Abstract)
class LASTDAY_API AExplosion : public AActor
{
	GENERATED_BODY()
};
