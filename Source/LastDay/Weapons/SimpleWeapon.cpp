// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimpleWeapon.h"
#include "CannonBall.h"
#include "LastDayVisuals.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

ASimpleWeapon::ASimpleWeapon()
{
	// Tick 只用来走开火冷却；瞄准在用它的人（炮塔 / 怪物）那边
	PrimaryActorTick.bCanEverTick = true;

	Damage = 10.0f;
	FireInterval = 0.5f;
	ProjectileSpeed = 2000.0f;
	CooldownRemaining = 0.0f;

	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	SetRootComponent(BarrelMesh);
	BarrelMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// 占位模型：引擎自带的长方体，X 轴就是炮管方向
	static ConstructorHelpers::FObjectFinder<UStaticMesh> defaultMesh(
		TEXT("/Engine/BasicShapes/Cube")
	);
	if (defaultMesh.Succeeded())
	{
		BarrelMesh->SetStaticMesh(defaultMesh.Object);
	}
}

void ASimpleWeapon::BeginPlay()
{
	Super::BeginPlay();

	// 金属外观（子类可以换色）
	ApplyLastDayMetalColor(BarrelMesh, GetBarrelColor());

	CooldownRemaining = 0.0f;

	// 宿主单位被销毁时自己也销毁，避免炮口/武器留在场上
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->OnDestroyed.AddDynamic(this, &ASimpleWeapon::OnOwnerDestroyed);
	}
}

void ASimpleWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CooldownRemaining > 0.0f)
	{
		CooldownRemaining = FMath::Max(0.0f, CooldownRemaining - DeltaSeconds);
	}
}

void ASimpleWeapon::OnOwnerDestroyed(AActor* DestroyedActor)
{
	Destroy();
}

FLinearColor ASimpleWeapon::GetBarrelColor() const
{
	return LastDayColors::Metal;
}

FRotator ASimpleWeapon::GetBarrelRotation() const
{
	// 炮管是根组件，它的相对旋转就是"相对挂载单位"的朝向
	return BarrelMesh ? BarrelMesh->GetRelativeRotation() : FRotator::ZeroRotator;
}

void ASimpleWeapon::SetBarrelRotation(const FRotator& NewRotation)
{
	if (BarrelMesh)
	{
		BarrelMesh->SetRelativeRotation(NewRotation);
	}
}

bool ASimpleWeapon::TryFire()
{
	if (CooldownRemaining > 0.0f || !ProjectileClass)
	{
		return false;
	}

	Fire();
	return true;
}

void ASimpleWeapon::Fire()
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileClass)
	{
		return;
	}

	// 发射方向就是炮管方向（瞄准已经由使用它的单位做好）
	ACannonBall* Projectile = FireProjectile(World, ProjectileClass, GetOwner(), GetActorLocation(), GetActorRotation(), ProjectileSpeed, Damage);

	// 武器自己是独立 Actor：工厂只会忽略"发射者"（宿主单位），不会忽略炮管本身，
	// 而炮弹正是在炮管里生成的，不忽略它就会一出膛就撞到自己的炮管（炸膛）。
	if (Projectile && Projectile->CollisionComp)
	{
		Projectile->CollisionComp->IgnoreActorWhenMoving(this, true);
	}

	CooldownRemaining = FMath::Max(FireInterval, 0.0f);
}
