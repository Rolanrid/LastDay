// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

/**
 * 武器基类。
 *
 * 暂时不做任何实现，只用来做继承判断（例如 Actor->IsA<AWeapon>()）。
 * 玩家武器（AShooterWeapon）、炮塔炮口（ACannon）都继承自它。
 */
UCLASS(Abstract)
class LASTDAY_API AWeapon : public AActor
{
	GENERATED_BODY()
};
