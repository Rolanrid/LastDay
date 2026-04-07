// Fill out your copyright notice in the Description page of Project Settings.

#include "TurretSocketComponent.h"
#include "Turret.h"
#include "BaseProjectile.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h" // 仅调试

UTurretSocketComponent::UTurretSocketComponent()
{
    // 设置组件的默认属性
    PrimaryComponentTick.bCanEverTick = false;
	fireRange = 800.0f;
	fireAngle = 60.0f;

    // 设置碰撞预设（可选）
    SetCollisionProfileName(TEXT("BlockAllDynamic"));

    // 设置默认模型（可选：使用引擎自带的圆柱体作为占位模型）
    static ConstructorHelpers::FObjectFinder<UStaticMesh> defaultMesh(
        TEXT("/Engine/BasicShapes/Cylinder")
    );
    if (defaultMesh.Succeeded())
    {
        SetStaticMesh(defaultMesh.Object);
    }
}

void UTurretSocketComponent::BeginPlay()
{
    Super::BeginPlay();
    ownerTurret = Cast<ATurret>(GetOwner());
    if (!projectileClass && ownerTurret)
    {
        projectileClass = ownerTurret->projectileClass; // 使用炮塔默认子弹
    }
}

bool UTurretSocketComponent::TryFire(float deltaTime)
{
    if (cooldownRemaining > 0.0f)
    {
        cooldownRemaining -= deltaTime;
    }

    // 寻找目标
    currentTarget = FindTargetInCone();
    if (currentTarget && cooldownRemaining <= 0.0f) {
        Fire();
        return true;
    }
    return false;
}

AActor* UTurretSocketComponent::FindTargetInCone()
{
    if (!ownerTurret) return nullptr;

    // 获取炮塔检测到的敌人列表（可从炮塔获取，也可自行检测）
    TArray<AActor*> candidates;
    // 此处简化：直接使用炮塔的检测结果，或者自行用Overlap检测
    // 为了演示，假设炮塔有一个公开的检测结果函数
    // 或者我们直接在组件内进行球形检测（这里选择自行检测以保持独立）
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    TArray<FOverlapResult> overlaps;
    FCollisionShape Shape = FCollisionShape::MakeSphere(fireRange);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());

    World->OverlapMultiByChannel(
        overlaps,
        GetComponentLocation(),
        FQuat::Identity,
        ECC_Pawn,
        Shape,
        QueryParams
    );

    // 过滤出敌人，并检查是否在扇形内
    FVector SocketLoc = GetComponentLocation();
    FVector Forward = GetForwardVector();

    for (const FOverlapResult& overlap : overlaps) {
        AActor* Actor = overlap.GetActor();
        if (Actor && Actor->IsA<AUnit>()) { // 过滤
            FVector DirToTarget = Actor->GetActorLocation() - SocketLoc;
            float Dist = DirToTarget.Size();
            if (Dist > fireRange)
            {
                continue;
            }
            DirToTarget.Normalize();
            float Dot = FVector::DotProduct(Forward, DirToTarget);
            float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));
            // if (Angle > fireAngle) 角度判定有怪问题，先不管
            // {
            //     continue;
            // }
            return Actor; // 返回第一个符合条件的
        }
    }
    return nullptr;
}

void UTurretSocketComponent::Fire()
{
    if (!projectileClass || !currentTarget) return;

    UWorld* World = GetWorld();
    if (!World) return;

    FVector SpawnLocation = GetComponentLocation();
    FRotator SpawnRotation = GetComponentRotation();

    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Fire!"));
	SpawnProjectile(World, Cast<AUnit>(GetOwner()), SpawnLocation, SpawnRotation, projectileSpeed);
    cooldownRemaining = ownerTurret ? ownerTurret->fireCooldown : 0.5f;

}