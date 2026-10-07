// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "SimpleWeapon.generated.h"

class ACannonBall;
class UStaticMeshComponent;

/**
 * 简易武器。
 *
 * 有一根炮管（根组件，X 轴就是发射方向）、伤害值、开火间隔、子弹类和冷却；
 * TryFire() 冷却好了就朝自己的前向发射一发炮弹。不做索敌、不做转向——
 * 谁用它（炮塔、怪物）谁负责瞄准，对准了再叫它开火。
 */
UCLASS(Abstract)
class LASTDAY_API ASimpleWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	ASimpleWeapon();

	/** 伤害值：开火时写进子弹，由子弹按官方伤害接口结算 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Damage;

	/** 开火间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0"))
	float FireInterval;

	/** 子弹类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<ACannonBall> ProjectileClass;

	/** 子弹初速度（cm/s） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0"))
	float ProjectileSpeed;

	/** 炮管模型（根组件）：X 轴就是发射方向 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BarrelMesh;

	/** 冷却结束就发射一发；返回本帧是否真的开火 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool TryFire();

	/** 剩余冷却时间（秒） */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetCooldownRemaining() const { return CooldownRemaining; }

	/** 相对挂载父级的朝向（就是炮管方向） */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FRotator GetBarrelRotation() const;

	/** 设置相对挂载父级的朝向（由使用它的单位每帧调用） */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetBarrelRotation(const FRotator& NewRotation);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 炮管颜色：子类可以改成别的颜色来区分阵营 */
	virtual FLinearColor GetBarrelColor() const;

	/** 宿主被销毁时自己也销毁 */
	UFUNCTION()
	void OnOwnerDestroyed(AActor* DestroyedActor);

	/** 剩余冷却（秒） */
	float CooldownRemaining;

	/** 立刻发射一发子弹（不看冷却，也不管有没有目标） */
	void Fire();
};
