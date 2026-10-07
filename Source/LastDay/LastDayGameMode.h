// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LastDayGameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class ALastDayGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALastDayGameMode();

	/** 关卡里没有矿脉时，自动在玩家出生点周围摆一圈（D1-4） */
	UPROPERTY(EditDefaultsOnly, Category = "Demo|Setup")
	bool bSpawnDemoCrystals = true;

	UPROPERTY(EditDefaultsOnly, Category = "Demo|Setup")
	int32 DemoCrystalCount = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Demo|Setup")
	float DemoCrystalRadius = 1200.0f;

	/** 关卡里没有炮塔时，自动摆一圈作为初始防御（E3-6） */
	UPROPERTY(EditDefaultsOnly, Category = "Demo|Setup")
	bool bSpawnInitialTurrets = true;

	UPROPERTY(EditDefaultsOnly, Category = "Demo|Setup")
	int32 InitialTurretCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Demo|Setup")
	float InitialTurretRadius = 800.0f;

protected:
	virtual void BeginPlay() override;

private:
	/** 关卡里没摆东西时的兜底摆放，方便直接跑 Demo */
	void SpawnInitialActors();
};



