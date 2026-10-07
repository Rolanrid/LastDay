// Fill out your copyright notice in the Description page of Project Settings.

#include "Turret.h"
#include "Cannon.h"
#include "CannonBall.h"
#include "LastDayVisuals.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "LastDay.h"

ATurret::ATurret()
{
    PrimaryActorTick.bCanEverTick = true;

    // ---- 索敌：XY 平面 10 米 / ±60°，Z 轴单独给一个高度区间 ----
    DetectionRadius = 1000.0f;
    DetectionAngle = 60.0f;
    MinHeightOffset = -200.0f;
    MaxHeightOffset = 1000.0f;

    // ---- 瞄准 ----
    RotationSpeed = 180.0f;
    bLimitAimRotation = true;
    MaxYawAngle = 120.0f;
    MinPitchAngle = -20.0f;
    MaxPitchAngle = 45.0f;
    AimTolerance = 5.0f;
    bDrawDebugAim = false;
    CurrentTarget = nullptr;

    // ---- 武器 ----
    CannonClass = ACannon::StaticClass();
    ProjectileClass = ACannonBall::StaticClass();

    // 炮塔主体：挂在 AUnit 的碰撞根上
    TurretRoot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
    TurretRoot->SetupAttachment(RootComponent);
    // 炮台有模型，直接用模型自身的简单碰撞（立方体），与模型完全贴合；
    // 关掉 AUnit 默认的碰撞盒，避免出现两个重叠的碰撞体。
    // 注意：受击判定仍然走模型碰撞，所以 AUnit::TakeDamage 照常生效。
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TurretRoot->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    // 加载引擎内置的长方体网格体
    static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeMeshAsset(
        TEXT("/Engine/BasicShapes/Cube")
    );
    if (cubeMeshAsset.Succeeded())
    {
        TurretRoot->SetStaticMesh(cubeMeshAsset.Object);
    }
}

void ATurret::BeginPlay()
{
    Super::BeginPlay();

    // 统一材质：炮台 = 银色金属
    ApplyLastDayMetalColor(TurretRoot, LastDayColors::Metal);

    // 生成自带的两个炮口
    SpawnDefaultCannons();

    // 蓝图里手动挂上来的炮口也一起收集
    TArray<AActor*> AttachedActors;
    GetAttachedActors(AttachedActors);
    for (AActor* AttachedActor : AttachedActors)
    {
        if (ACannon* AttachedCannon = Cast<ACannon>(AttachedActor))
        {
            AddCannon(AttachedCannon);
        }
    }

    UE_LOG(LogLastDay, Log, TEXT("[Turret] %s 就绪，炮口 %d 个"), *GetName(), Cannons.Num());
}

void ATurret::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UpdateTarget();        // 1. 索敌：XY 平面角度/距离 + Z 轴范围
    UpdateAim(DeltaTime);  // 2. 把炮口转向目标
    UpdateFiring();        // 3. 炮口对准了就开火（开火间隔由炮口自己算）

    if (bDrawDebugAim)
    {
        DrawDebugAim();
    }
}

void ATurret::SpawnDefaultCannons()
{
    UWorld* World = GetWorld();
    if (!World || !CannonClass)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 两个炮口：左右各一个。X 轴是炮管长度方向，也就是子弹的出膛方向。
    for (int32 Index = 0; Index < 2; ++Index)
    {
        ACannon* Cannon = World->SpawnActor<ACannon>(CannonClass, GetActorTransform(), SpawnParams);
        if (!Cannon)
        {
            continue;
        }

        // 挂到炮塔底座上
        Cannon->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
        Cannon->SetActorRelativeLocation(FVector(10.0f, Index * 40.0f - 20.0f, 60.0f));
        // 不旋转：炮口前向（X）就是炮管方向，转向由炮塔在 UpdateAim 里驱动
        Cannon->SetActorRelativeRotation(FRotator::ZeroRotator);
        // X 为炮管长度（1.0 → 100），Y/Z 为截面粗细（0.2 → 20）
        Cannon->SetActorRelativeScale3D(FVector(1.0f, 0.2f, 0.2f));

        AddCannon(Cannon);
    }
}

void ATurret::AddCannon(ACannon* Cannon)
{
    if (!Cannon || Cannons.Contains(Cannon))
    {
        return;
    }

    Cannon->OwnerTurret = this;
    if (!Cannon->ProjectileClass)
    {
        Cannon->ProjectileClass = ProjectileClass;   // 下发炮塔的默认子弹
    }
    Cannons.Add(Cannon);
}

// =====================================================
// 索敌
// =====================================================

void ATurret::UpdateTarget()
{
    // 旧目标只要还合格就继续锁定，避免炮口转到一半把目标丢掉
    if (!IsTargetCandidate(CurrentTarget))
    {
        CurrentTarget = FindTarget();
    }
}

bool ATurret::IsTargetCandidate(const AActor* Actor) const
{
    if (!IsValid(Actor) || Actor == this)
    {
        return false;
    }

    // 只打单位，而且不打自己阵营的（怪物阵营和玩家阵营互为目标）
    const AUnit* OtherUnit = Cast<AUnit>(Actor);
    if (!OtherUnit)
    {
        return false;
    }
    if (OtherUnit->GetTeam() == GetTeam())
    {
        return false;
    }

    const FVector Delta = Actor->GetActorLocation() - GetActorLocation();

    // Z 轴单独判断：只在这个高度区间内才算目标
    if (Delta.Z < MinHeightOffset || Delta.Z > MaxHeightOffset)
    {
        return false;
    }

    // XY 平面：先看距离
    const FVector DeltaXY(Delta.X, Delta.Y, 0.0f);
    const float DistXY = DeltaXY.Size();
    if (DistXY > DetectionRadius || DistXY <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    // XY 平面：再看与炮塔前向的夹角
    const FVector ForwardXY = GetActorForwardVector().GetSafeNormal2D();
    const float CosAngle = FVector::DotProduct(ForwardXY, DeltaXY / DistXY);
    const float AngleXY = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosAngle, -1.0f, 1.0f)));

    return AngleXY <= DetectionAngle;
}

AActor* ATurret::FindTarget()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // 先用一个能罩住"XY 半径 + Z 范围"的球做粗筛，再逐个精确判断
    const float ZExtent = FMath::Max(FMath::Abs(MinHeightOffset), FMath::Abs(MaxHeightOffset));
    const float QueryRadius = FMath::Sqrt(FMath::Square(DetectionRadius) + FMath::Square(ZExtent)) + KINDA_SMALL_NUMBER;

    TArray<FOverlapResult> Overlaps;
    FCollisionShape Shape = FCollisionShape::MakeSphere(QueryRadius);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TurretFindTarget), false, this);

    World->OverlapMultiByChannel(
        Overlaps,
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        Shape,
        QueryParams
    );

    // 合格目标里挑最近的一个
    AActor* BestTarget = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Actor = Overlap.GetActor();
        if (!IsTargetCandidate(Actor))
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestTarget = Actor;
        }
    }

    return BestTarget;
}

// =====================================================
// 瞄准与开火
// =====================================================

void ATurret::UpdateAim(float DeltaTime)
{
    if (DeltaTime <= 0.0f)
    {
        return;
    }

    const float MaxStep = FMath::Max(RotationSpeed, 0.0f) * DeltaTime;
    const FTransform BaseTransform = GetActorTransform();

    for (const TObjectPtr<ACannon>& CannonPtr : Cannons)
    {
        ACannon* Cannon = CannonPtr;
        if (!Cannon)
        {
            continue;
        }

        // 没有目标就停在当前角度
        FRotator Desired = Cannon->GetBarrelRotation();

        if (CurrentTarget)
        {
            const FVector WorldDir = (CurrentTarget->GetActorLocation() - Cannon->GetActorLocation()).GetSafeNormal();
            if (!WorldDir.IsNearlyZero())
            {
                // 换算成相对炮塔底座的方向，限角才有意义
                const FVector LocalDir = BaseTransform.InverseTransformVectorNoScale(WorldDir);
                Desired = LocalDir.Rotation();
                if (bLimitAimRotation)
                {
                    Desired.Yaw = FMath::Clamp(Desired.Yaw, -MaxYawAngle, MaxYawAngle);
                    Desired.Pitch = FMath::Clamp(Desired.Pitch, MinPitchAngle, MaxPitchAngle);
                }
                Desired.Roll = 0.0f;
            }
        }

        // 按 RotationSpeed 限速逼近目标角度；Yaw 用 FindDeltaAngleDegrees 处理跨 ±180 的情况
        const FRotator Current = Cannon->GetBarrelRotation();
        FRotator NewRelative = Current;
        NewRelative.Yaw = Current.Yaw + FMath::Clamp(FMath::FindDeltaAngleDegrees(Current.Yaw, Desired.Yaw), -MaxStep, MaxStep);
        NewRelative.Pitch = Current.Pitch + FMath::Clamp(Desired.Pitch - Current.Pitch, -MaxStep, MaxStep);
        NewRelative.Roll = 0.0f;

        Cannon->SetBarrelRotation(NewRelative);
    }
}

void ATurret::UpdateFiring()
{
    if (!CurrentTarget)
    {
        return;
    }

    for (const TObjectPtr<ACannon>& CannonPtr : Cannons)
    {
        ACannon* Cannon = CannonPtr;
        if (!Cannon)
        {
            continue;
        }

        // 炮口转到容差内才开火，避免边转边乱射
        if (GetAimAngleToTarget(*Cannon) <= AimTolerance)
        {
            Cannon->TryFire();
        }
    }
}

float ATurret::GetAimAngleToTarget(const ACannon& Cannon) const
{
    if (!CurrentTarget)
    {
        return 180.0f;
    }

    const FVector Dir = (CurrentTarget->GetActorLocation() - Cannon.GetActorLocation()).GetSafeNormal();
    if (Dir.IsNearlyZero())
    {
        return 180.0f;
    }

    const float Dot = FVector::DotProduct(Cannon.GetActorRotation().Vector(), Dir);
    return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
}

void ATurret::DrawDebugAim()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector Origin = GetActorLocation();
    const FVector ForwardXY = GetActorForwardVector().GetSafeNormal2D();

    // XY 检测扇区（左右两条边界）
    const FVector LeftEdge = ForwardXY.RotateAngleAxis(-DetectionAngle, FVector::UpVector) * DetectionRadius;
    const FVector RightEdge = ForwardXY.RotateAngleAxis(DetectionAngle, FVector::UpVector) * DetectionRadius;
    DrawDebugLine(World, Origin, Origin + LeftEdge, FColor::Silver, false, -1.0f, 0, 1.0f);
    DrawDebugLine(World, Origin, Origin + RightEdge, FColor::Silver, false, -1.0f, 0, 1.0f);
    DrawDebugCircle(World, Origin, DetectionRadius, 32, FColor(80, 80, 80), false, -1.0f, 0, 1.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);

    // Z 轴高度范围
    DrawDebugLine(World, Origin + FVector(0, 0, MinHeightOffset), Origin + FVector(0, 0, MaxHeightOffset), FColor(0, 128, 255), false, -1.0f, 0, 2.0f);

    // 当前目标
    if (CurrentTarget)
    {
        DrawDebugSphere(World, CurrentTarget->GetActorLocation(), 40.0f, 12, FColor::Green, false, -1.0f, 0, 2.0f);
        for (const TObjectPtr<ACannon>& CannonPtr : Cannons)
        {
            if (CannonPtr)
            {
                DrawDebugLine(World, CannonPtr->GetActorLocation(), CurrentTarget->GetActorLocation(), FColor::Green, false, -1.0f, 0, 1.5f);
            }
        }
    }
}
