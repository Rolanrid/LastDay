// Fill out your copyright notice in the Description page of Project Settings.

#include "TurretSocketComponent.h"
#include "Turret.h"
#include "BaseProjectile.h"
#include "Unit.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h" // 仅调试

UTurretSocketComponent::UTurretSocketComponent()
{
    // 发射口要每帧给自己转向，所以打开组件 Tick
    PrimaryComponentTick.bCanEverTick = true;

    FireRange = 800.0f;
    FireAngle = 60.0f;
    ProjectileSpeed = 2000.0f;

    // 转向默认值：限速转过去，并限制在底座的一个合理范围内
    RotationSpeed = 180.0f;
    bLimitRotation = true;
    MaxYawAngle = 120.0f;
    MinPitchAngle = -20.0f;
    MaxPitchAngle = 45.0f;
    AimTolerance = 5.0f;
    bDrawDebugAim = false;

    CooldownRemaining = 0.0f;
    CurrentTarget = nullptr;

    // 设置碰撞预设（可选）
    SetCollisionProfileName(TEXT("BlockAllDynamic"));

    // 设置默认模型（可选：使用引擎自带的长方体作为占位模型）
    static ConstructorHelpers::FObjectFinder<UStaticMesh> defaultMesh(
        TEXT("/Engine/BasicShapes/Cube")
    );
    if (defaultMesh.Succeeded())
    {
        SetStaticMesh(defaultMesh.Object);
    }
}

void UTurretSocketComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerTurret = Cast<ATurret>(GetOwner());
    if (!ProjectileClass && OwnerTurret)
    {
        ProjectileClass = OwnerTurret->ProjectileClass; // 使用炮塔默认子弹
    }

    CurrentTarget = nullptr;
    CooldownRemaining = 0.0f;
}

void UTurretSocketComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    TryFire(DeltaTime);
}

bool UTurretSocketComponent::TryFire(float DeltaTime)
{
    // 1. 刷新目标（旧目标失效才重新搜索，避免转向过程中把目标弄丢）
    UpdateTarget();

    // 2. 朝目标实时转向
    UpdateAim(DeltaTime);

    // 3. 冷却结束后，对准目标才开火
    const bool bFired = UpdateFiring(DeltaTime);

    if (bDrawDebugAim)
    {
        const FVector Origin = GetComponentLocation();
        if (CurrentTarget)
        {
            DrawDebugLine(GetWorld(), Origin, CurrentTarget->GetActorLocation(), FColor::Green, false, -1.0f, 0, 1.5f);
        }
        else
        {
            DrawDebugLine(GetWorld(), Origin, Origin + GetForwardVector() * FireRange, FColor::Silver, false, -1.0f, 0, 1.5f);
        }
    }

    return bFired;
}

void UTurretSocketComponent::UpdateTarget()
{
    if (!IsTargetCandidate(CurrentTarget))
    {
        CurrentTarget = FindTargetInCone();
    }
}

AActor* UTurretSocketComponent::FindTargetInCone()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // 组件独立做球形检测，避免对炮塔的检测结果产生顺序依赖
    TArray<FOverlapResult> overlaps;
    FCollisionShape Shape = FCollisionShape::MakeSphere(FireRange);
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

    // 扇区内的候选目标里挑最近的一个
    const FVector SocketLoc = GetComponentLocation();
    AActor* BestTarget = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    for (const FOverlapResult& overlap : overlaps)
    {
        AActor* Actor = overlap.GetActor();
        if (!IsTargetCandidate(Actor))
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(SocketLoc, Actor->GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestTarget = Actor;
        }
    }

    return BestTarget;
}

bool UTurretSocketComponent::IsTargetCandidate(const AActor* Actor) const
{
    if (!IsValid(Actor) || Actor == GetOwner())
    {
        return false;
    }

    // 目前只把 Unit 当作目标
    if (!Actor->IsA<AUnit>())
    {
        return false;
    }

    const FVector SocketLoc = GetComponentLocation();
    FVector DirToTarget = Actor->GetActorLocation() - SocketLoc;
    const float Dist = DirToTarget.Size();
    if (Dist > FireRange || Dist <= KINDA_SMALL_NUMBER)
    {
        return false;
    }
    DirToTarget /= Dist;

    // 用底座前向判断扇区，这样炮口自己转动的过程中不会把目标丢掉
    const float Dot = FVector::DotProduct(GetBaseForwardVector(), DirToTarget);
    const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
    return Angle <= FireAngle;
}

void UTurretSocketComponent::UpdateAim(float DeltaTime)
{
    if (!CurrentTarget || DeltaTime <= 0.0f)
    {
        return;
    }

    const FVector SocketLoc = GetComponentLocation();
    const FVector WorldDir = (CurrentTarget->GetActorLocation() - SocketLoc).GetSafeNormal();
    if (WorldDir.IsNearlyZero())
    {
        return;
    }

    // 换算成相对炮塔底座的方向，限角才有意义
    const USceneComponent* Parent = GetAttachParent();
    const FVector LocalDir = Parent
        ? Parent->GetComponentTransform().InverseTransformVectorNoScale(WorldDir)
        : WorldDir;

    FRotator Desired = LocalDir.Rotation(); // 由方向求出的旋转，Roll 为 0
    if (bLimitRotation)
    {
        Desired.Yaw = FMath::Clamp(Desired.Yaw, -MaxYawAngle, MaxYawAngle);
        Desired.Pitch = FMath::Clamp(Desired.Pitch, MinPitchAngle, MaxPitchAngle);
    }
    Desired.Roll = 0.0f;

    // 按 RotationSpeed 限速逼近目标角度；Yaw 用 FindDeltaAngleDegrees 处理跨 ±180 的情况
    const float MaxStep = FMath::Max(RotationSpeed, 0.0f) * DeltaTime;
    const FRotator Current = GetRelativeRotation();

    FRotator NewRelative = Current;
    NewRelative.Yaw = Current.Yaw + FMath::Clamp(FMath::FindDeltaAngleDegrees(Current.Yaw, Desired.Yaw), -MaxStep, MaxStep);
    NewRelative.Pitch = Current.Pitch + FMath::Clamp(Desired.Pitch - Current.Pitch, -MaxStep, MaxStep);
    NewRelative.Roll = 0.0f;

    SetRelativeRotation(NewRelative);
}

bool UTurretSocketComponent::UpdateFiring(float DeltaTime)
{
    if (CooldownRemaining > 0.0f)
    {
        CooldownRemaining -= DeltaTime;
    }

    if (!CurrentTarget || CooldownRemaining > 0.0f)
    {
        return false;
    }

    // 还没转到位就先不开火
    if (GetAimAngleToTarget() > AimTolerance)
    {
        return false;
    }

    Fire();
    return true;
}

FVector UTurretSocketComponent::GetBaseForwardVector() const
{
    const USceneComponent* Parent = GetAttachParent();
    return Parent ? Parent->GetForwardVector() : GetForwardVector();
}

float UTurretSocketComponent::GetAimAngleToTarget() const
{
    if (!CurrentTarget)
    {
        return 180.0f;
    }

    const FVector Dir = (CurrentTarget->GetActorLocation() - GetComponentLocation()).GetSafeNormal();
    if (Dir.IsNearlyZero())
    {
        return 180.0f;
    }

    const float Dot = FVector::DotProduct(GetForwardVector(), Dir);
    return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
}

AActor* UTurretSocketComponent::GetCurrentTarget() const
{
    return CurrentTarget;
}

void UTurretSocketComponent::Fire()
{
    if (!ProjectileClass || !CurrentTarget)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector SpawnLocation = GetComponentLocation();
    const FRotator SpawnRotation = GetComponentRotation(); // 已经朝向目标

    FireProjectile(World, ProjectileClass, Cast<AUnit>(GetOwner()), SpawnLocation, SpawnRotation, ProjectileSpeed);
    CooldownRemaining = OwnerTurret ? OwnerTurret->FireCooldown : 0.5f;
}
