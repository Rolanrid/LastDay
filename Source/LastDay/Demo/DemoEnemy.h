// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DemoTypes.h"
#include "DemoEnemy.generated.h"

class UStaticMeshComponent;
class AMonster;

/**
 * H 模块用的最小敌人（G 模块的占位实现）：
 * 红色球体，自动寻找最近的建筑/炮塔并靠近，进入攻击范围后按间隔造成伤害；没有建筑时改打玩家。
 */
UCLASS()
class LASTDAY_API ADemoEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	ADemoEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** G4-1：标准受击入口，血量归零则销毁并通知波次系统 */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, Category = "Enemy")
	float MaxHealth = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy")
	float AttackDamage = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy")
	float MoveSpeed = 300.0f;

	/** 攻击范围（cm，3 米 = 300） */
	UPROPERTY(EditAnywhere, Category = "Enemy")
	float AttackRange = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy")
	float AttackInterval = 1.0f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BodyMesh;

	/** 当前攻击目标（建筑/炮塔，或玩家） */
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	float Health = 50.0f;
	float AttackCooldown = 0.0f;
	float RepathCooldown = 0.0f;

	/** G2-1/G2-3/G5-2：选最近的建筑，没有就选玩家 */
	void AcquireTarget();

	/** G3：靠近并攻击 */
	void UpdateCombat(float DeltaSeconds);
};

/**
 * H2：刷怪点。夜晚阶段开始时按波次数量在自带的一圈刷新点上逐个生成敌人。
 * 关卡里没有手动摆放时，GameMode 会自动生成一个（8 个环绕刷新点）。
 */
UCLASS()
class LASTDAY_API AEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawner();

	virtual void BeginPlay() override;

	/** 要刷的怪物类型 */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	TSubclassOf<AMonster> MonsterClass;

	/** 刷新点数量（H2-1，建议 8-12） */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	int32 SpawnPointCount = 8;

	/** 刷新点环绕半径（cm） */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	float SpawnRadius = 2500.0f;

	/** 逐个生成的间隔下限/上限（秒，H2-3） */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	float MinSpawnInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	float MaxSpawnInterval = 2.0f;

protected:
	/** 监听阶段切换：一进入夜晚就开始刷怪 */
	UFUNCTION()
	void HandleDayPhaseChanged(EDayPhase NewPhase);

	void BuildSpawnPoints();
	void SpawnOne();

	/** 本波刷新点（世界坐标） */
	TArray<FVector> SpawnPoints;

	int32 SpawnedThisWave = 0;
	int32 WaveTotal = 0;

	FTimerHandle SpawnTimer;
};
