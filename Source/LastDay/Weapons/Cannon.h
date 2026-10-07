// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SimpleWeapon.h"
#include "Cannon.generated.h"

class ATurret;

/**
 * 炮口（炮塔武器）。
 *
 * 就是个简易武器（ASimpleWeapon）：存伤害值、管开火间隔、发射子弹。
 * 索敌、目标选择、炮管转向全部由炮塔（ATurret）负责；炮塔把炮口转到位后调用 TryFire()。
 * 它由炮塔在 BeginPlay 时生成并挂到炮塔底座上。
 */
UCLASS()
class LASTDAY_API ACannon : public ASimpleWeapon
{
	GENERATED_BODY()

public:
	ACannon();

	/** 所属炮塔（由炮塔在收集炮口时写入） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	ATurret* OwnerTurret;

protected:
	virtual void BeginPlay() override;
};
