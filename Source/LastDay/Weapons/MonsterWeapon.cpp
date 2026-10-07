// Copyright Epic Games, Inc. All Rights Reserved.

#include "MonsterWeapon.h"
#include "CannonBall.h"
#include "LastDayVisuals.h"

AMonsterWeapon::AMonsterWeapon()
{
	Damage = 8.0f;
	FireInterval = 1.5f;
	ProjectileSpeed = 1600.0f;
	// 没有炮塔给它下发子弹类，自己带一个
	ProjectileClass = ACannonBall::StaticClass();
}

FLinearColor AMonsterWeapon::GetBarrelColor() const
{
	return LastDayColors::Monster;
}
