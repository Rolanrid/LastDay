// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

/**
 * 抛射物基类。
 *
 * 暂时不做任何实现，只用来做继承判断（例如 OtherActor->IsA<AProjectile>()）。
 * 炮塔炮弹（ACannonBall）、玩家子弹（AShooterProjectile）都继承自它。
 */
UCLASS(Abstract)
class LASTDAY_API AProjectile : public AActor
{
	GENERATED_BODY()
};
