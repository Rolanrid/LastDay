// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Unit.h"
#include "DemoBuildings.generated.h"

class UStaticMeshComponent;

/**
 * D1：源晶矿脉。被子弹命中即"被采集"，采集量直接进玩家资源；
 * 储量耗尽后自我销毁（矿脉枯竭）。
 */
UCLASS()
class LASTDAY_API ASourceCrystal : public AActor
{
	GENERATED_BODY()

public:
	ASourceCrystal();

	/** 标准受击入口：被子弹打中 = 被采集 10 点 */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** 被采集：储量减少并把采集量加到玩家资源上 */
	UFUNCTION(BlueprintCallable, Category = "Crystal")
	void Gather(int32 Amount);

	/** D1-2：最大储量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crystal")
	float MaxAmount = 200.0f;

	/** D1-2：当前储量 */
	UPROPERTY(BlueprintReadOnly, Category = "Crystal")
	float CurrentAmount = 200.0f;

	/** 每次被击中的采集量（D2-1 用 10） */
	UPROPERTY(EditAnywhere, Category = "Crystal")
	int32 GatherPerHit = 10;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;
};

/**
 * D3：采集器建筑。每 ProduceInterval 秒在 GatherRadius 内找一座矿脉，扣 1 点并给玩家 +1。
 * 继承 AUnit，所以有血量、能被射击、被拆掉时会有爆炸表现。
 */
UCLASS()
class LASTDAY_API ACollectorBuilding : public AUnit
{
	GENERATED_BODY()

public:
	ACollectorBuilding();

	/** 采集间隔（秒） */
	UPROPERTY(EditAnywhere, Category = "Collector")
	float ProduceInterval = 1.0f;

	/** 采集半径（cm，3 米 = 300） */
	UPROPERTY(EditAnywhere, Category = "Collector")
	float GatherRadius = 300.0f;

	/** 每次采集量 */
	UPROPERTY(EditAnywhere, Category = "Collector")
	int32 ProducePerTick = 1;

protected:
	virtual void BeginPlay() override;

	/** 定时器回调：采集一次 */
	void Produce();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

	FTimerHandle ProduceTimer;
};
