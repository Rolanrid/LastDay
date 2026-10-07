// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "MiningDamageType.generated.h"

/**
 * 采矿伤害类型。
 *
 * 只有带这个类型的伤害才能从矿脉上采集资源；普通枪械用的是 UDamageType，
 * 所以对着矿脉开枪不会掉矿。反过来，单位（AUnit）也不吃采矿伤害。
 */
UCLASS()
class LASTDAY_API UMiningDamageType : public UDamageType
{
	GENERATED_BODY()
};
