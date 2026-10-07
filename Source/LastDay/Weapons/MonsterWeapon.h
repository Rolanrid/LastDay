// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SimpleWeapon.h"
#include "MonsterWeapon.generated.h"

/**
 * 怪物用的简易武器：和炮口同源（都是 ASimpleWeapon），
 * 只是默认数值不同、炮管是红色的，用来区分阵营。
 */
UCLASS()
class LASTDAY_API AMonsterWeapon : public ASimpleWeapon
{
	GENERATED_BODY()

public:
	AMonsterWeapon();

protected:
	virtual FLinearColor GetBarrelColor() const override;
};
