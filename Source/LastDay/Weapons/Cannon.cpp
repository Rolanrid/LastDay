// Fill out your copyright notice in the Description page of Project Settings.

#include "Cannon.h"
#include "CannonBall.h"
#include "Turret.h"

ACannon::ACannon()
{
	OwnerTurret = nullptr;
}

void ACannon::BeginPlay()
{
	Super::BeginPlay();

	OwnerTurret = Cast<ATurret>(GetOwner());
	if (!ProjectileClass && OwnerTurret)
	{
		ProjectileClass = OwnerTurret->ProjectileClass;   // 用炮塔的默认子弹
	}
}
