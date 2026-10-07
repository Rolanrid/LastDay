// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Unit.h"
#include "Monster.generated.h"

class AMonsterWeapon;
class UStaticMeshComponent;

/**
 * 怪物（敌人）。
 *
 * 外观暂时用红色的三角锥（引擎自带 Cone 白模）代替；
 * 行为：在一定范围内找最近的敌人（敌方 AUnit + 玩家本人），
 * 沿导航网格的路径走过去（不会穿墙）、把武器转向它并开火。
 *
 * 为了防止来回摇头：目标锁定后会保持 TargetSwitchCooldown 秒，
 * 冷却到点之后也要求新目标明显更近（SwitchDistanceAdvantage）才会换。
 */
UCLASS()
class LASTDAY_API AMonster : public AUnit
{
	GENERATED_BODY()

public:
	AMonster();

	/** 索敌半径（cm）：只找这个范围内的目标 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0.0"))
	float DetectionRadius;

	/** 移动速度（cm/s） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0.0"))
	float MoveSpeed;

	/** 攻击距离（cm）：走到离目标这么近就停下来开火 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0.0"))
	float AttackRange;

	/** 换目标的最小间隔（秒）：这就是防"频繁摇头"的冷却 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0.0"))
	float TargetSwitchCooldown;

	/** 换目标需要的距离优势：新目标要近到旧目标的这个比例以内才换（0.7 = 得近 30% 以上） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SwitchDistanceAdvantage;

	/** 重新寻路的间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Movement", meta = (ClampMin = "0.1"))
	float RepathInterval;

	/** 是否用导航网格寻路（没有导航数据时自动退回带避障的转向，一样不会穿墙） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Movement")
	bool bUseNavigation;

	/** 武器类（默认 AMonsterWeapon，可换成蓝图子类） */
	UPROPERTY(EditDefaultsOnly, Category = "Monster|Combat")
	TSubclassOf<AMonsterWeapon> WeaponClass;

	/** 武器转向速度（度/秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0.0"))
	float RotationSpeed;

	/** 瞄准容差（度）：武器与目标夹角小于它才开火 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float AimTolerance;

	/** 当前锁定的目标（可能为空） */
	UFUNCTION(BlueprintPure, Category = "Monster")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 被打死时通知波次系统 */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** 外观（红色三角锥） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BodyMesh;

	/** 手里的武器（BeginPlay 时生成并挂在自己身上） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster")
	TObjectPtr<AMonsterWeapon> Weapon;

private:
	/** 当前目标 */
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	/** 换目标冷却剩余（秒） */
	float TargetSwitchCooldownRemaining = 0.0f;

	/** 距离下次可以重新寻路还有多久（秒） */
	float RepathCooldown = 0.0f;

	/** 当前路径（导航网格算出来的世界坐标点） */
	TArray<FVector> PathPoints;

	/** 走到路径的第几个点了 */
	int32 PathIndex = 0;

	/** 移动模式的提示只打一次日志 */
	bool bLoggedMovementMode = false;

	/** 第一次开火只打一次日志 */
	bool bLoggedFirstShot = false;

	/** 是不是"敌人"：敌方 AUnit，或者还活着的玩家本人 */
	bool IsEnemyTarget(const AActor* Actor) const;

	/** 找范围内最近的敌人 */
	AActor* FindNearestEnemy();

	/** 刷新目标（带换目标冷却和距离优势判定） */
	void UpdateTarget(float DeltaSeconds);

	/** 朝目标移动 */
	void UpdateMovement(float DeltaSeconds);

	/** 用导航网格算一条路径；没有导航数据就返回 false */
	bool BuildPath(const FVector& Goal);

	/** 求这一步该往哪个方向走（没有路径时用带避障的转向） */
	FVector ComputeMoveDirection(const FVector& Goal) const;

	/** 把武器转向目标 */
	void UpdateAim(float DeltaSeconds);

	/** 对准了就开火 */
	void UpdateFiring();
};
