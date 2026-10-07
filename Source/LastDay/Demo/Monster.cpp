// Copyright Epic Games, Inc. All Rights Reserved.

#include "Monster.h"
#include "MonsterWeapon.h"
#include "LastDayCharacter.h"
#include "LastDayGameState.h"
#include "LastDayVisuals.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "LastDay.h"

AMonster::AMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	// 怪物阵营
	SetTeam(static_cast<uint64_t>(EUnitTeam::Monster));

	// 刷怪圈半径 2500，基地最远再偏出 800 左右，索敌范围要盖住这个量级
	DetectionRadius = 4000.0f;
	MoveSpeed = 300.0f;
	AttackRange = 900.0f;
	TargetSwitchCooldown = 2.0f;
	SwitchDistanceAdvantage = 0.7f;
	RepathInterval = 0.6f;
	bUseNavigation = true;

	WeaponClass = AMonsterWeapon::StaticClass();
	RotationSpeed = 120.0f;
	AimTolerance = 6.0f;

	// 受击体比建筑小一圈（怪物是三角锥，不是方块）
	CollisionComponent->InitBoxExtent(FVector(40.0f, 40.0f, 50.0f));

	// 外观：红色三角锥（引擎自带 Cone 白模，侧面看就是三角形）
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetCollisionProfileName(TEXT("NoCollision"));   // 受击判定交给 AUnit 的碰撞盒

	static ConstructorHelpers::FObjectFinder<UStaticMesh> coneMesh(TEXT("/Engine/BasicShapes/Cone"));
	if (coneMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(coneMesh.Object);
		BodyMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.4f));
	}
}

void AMonster::BeginPlay()
{
	Super::BeginPlay();

	// 红色三角形（暂代外观）
	ApplyLastDayColor(BodyMesh, LastDayColors::Monster);

	// 生成武器并挂在自己身上
	UWorld* World = GetWorld();
	if (World && WeaponClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AMonsterWeapon* SpawnedWeapon = World->SpawnActor<AMonsterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);
		if (SpawnedWeapon)
		{
			SpawnedWeapon->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
			SpawnedWeapon->SetActorRelativeLocation(FVector(30.0f, 0.0f, 60.0f));
			// 比炮管细一点，像个手持武器
			SpawnedWeapon->SetActorRelativeScale3D(FVector(0.5f, 0.12f, 0.12f));
			Weapon = SpawnedWeapon;
		}
	}

	UE_LOG(LogLastDay, Log, TEXT("[Monster] %s 就绪（阵营 %llu，索敌 %.0f，武器 %s）"),
		*GetName(), GetTeam(), DetectionRadius, Weapon ? TEXT("有") : TEXT("无"));
}

void AMonster::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateTarget(DeltaSeconds);
	UpdateMovement(DeltaSeconds);
	UpdateAim(DeltaSeconds);
	UpdateFiring();
}

float AMonster::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 被打死（血量归零）时告诉波次系统
	if (GetHealth() <= 0.0f)
	{
		if (ALastDayGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ALastDayGameState>() : nullptr)
		{
			GameState->NotifyEnemyKilled();
		}
	}

	return ActualDamage;
}

// =====================================================
// 索敌
// =====================================================

bool AMonster::IsEnemyTarget(const AActor* Actor) const
{
	if (!IsValid(Actor) || Actor == this)
	{
		return false;
	}

	// 敌方单位（建筑、炮塔……）
	if (const AUnit* OtherUnit = Cast<AUnit>(Actor))
	{
		return OtherUnit->GetTeam() != GetTeam();
	}

	// 玩家本人：不是 AUnit，但同样是敌人；死了就不再当目标
	if (const ALastDayCharacter* PlayerCharacter = Cast<ALastDayCharacter>(Actor))
	{
		return PlayerCharacter->CurrentHealth > 0.0f;
	}

	return false;
}

AActor* AMonster::FindNearestEnemy()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AActor* BestTarget = nullptr;
	float BestDistSq = FMath::Square(DetectionRadius);

	// 敌方单位
	for (TActorIterator<AUnit> It(World); It; ++It)
	{
		AUnit* Unit = *It;
		if (!IsEnemyTarget(Unit))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Unit->GetActorLocation(), GetActorLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Unit;
		}
	}

	// 玩家
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (IsEnemyTarget(PlayerPawn))
		{
			const float DistSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), GetActorLocation());
			if (DistSq <= BestDistSq)
			{
				BestTarget = PlayerPawn;
			}
		}
	}

	return BestTarget;
}

void AMonster::UpdateTarget(float DeltaSeconds)
{
	TargetSwitchCooldownRemaining = FMath::Max(0.0f, TargetSwitchCooldownRemaining - DeltaSeconds);

	// 当前目标还有效吗（是敌人 + 还在范围内）
	bool bCurrentValid = false;
	if (IsValid(CurrentTarget) && IsEnemyTarget(CurrentTarget))
	{
		bCurrentValid = FVector::DistSquared(CurrentTarget->GetActorLocation(), GetActorLocation()) <= FMath::Square(DetectionRadius);
	}

	// 目标还有效、冷却没到 → 继续用当前目标（防摇头的关键）
	if (bCurrentValid && TargetSwitchCooldownRemaining > 0.0f)
	{
		return;
	}

	AActor* BestTarget = FindNearestEnemy();

	// 冷却到点后，也只在"新目标明显更近"时才换
	if (bCurrentValid && BestTarget && BestTarget != CurrentTarget)
	{
		const float CurrentDistSq = FVector::DistSquared(CurrentTarget->GetActorLocation(), GetActorLocation());
		const float BestDistSq = FVector::DistSquared(BestTarget->GetActorLocation(), GetActorLocation());
		if (BestDistSq > CurrentDistSq * FMath::Square(SwitchDistanceAdvantage))
		{
			BestTarget = CurrentTarget;
		}
	}

	if (BestTarget != CurrentTarget)
	{
		CurrentTarget = BestTarget;
		TargetSwitchCooldownRemaining = FMath::Max(TargetSwitchCooldown, 0.1f);

		// 目标换了，路径作废
		PathPoints.Reset();
		PathIndex = 0;
		RepathCooldown = 0.0f;
	}
	else
	{
		// 这轮扫描过了，隔一会儿再扫，别每帧全场景遍历
		TargetSwitchCooldownRemaining = FMath::Max(TargetSwitchCooldownRemaining, TargetSwitchCooldown * 0.5f);
	}
}

// =====================================================
// 移动（导航网格路径 + 避障转向）
// =====================================================

void AMonster::UpdateMovement(float DeltaSeconds)
{
	if (!CurrentTarget || DeltaSeconds <= 0.0f || MoveSpeed <= 0.0f)
	{
		return;
	}

	const FVector Goal = CurrentTarget->GetActorLocation();
	FVector ToGoal = Goal - GetActorLocation();
	ToGoal.Z = 0.0f;
	if (ToGoal.Size() <= AttackRange)
	{
		PathPoints.Reset();   // 已经在攻击距离内，停下来开火
		return;
	}

	RepathCooldown = FMath::Max(0.0f, RepathCooldown - DeltaSeconds);

	// 需要（重新）寻路：没路径、走到头了、或者路径过期
	if (bUseNavigation && RepathCooldown <= 0.0f && (PathPoints.Num() == 0 || PathIndex >= PathPoints.Num()))
	{
		RepathCooldown = FMath::Max(RepathInterval, 0.1f);
		if (!BuildPath(Goal))
		{
			PathPoints.Reset();
			PathIndex = 0;
		}
	}

	// 有路径就沿路径走，没有就直接朝目标走（方向由避障转向给出）
	FVector MoveTarget = Goal;
	if (PathPoints.IsValidIndex(PathIndex))
	{
		MoveTarget = PathPoints[PathIndex];
		if (FVector::DistSquared2D(MoveTarget, GetActorLocation()) <= FMath::Square(80.0f))
		{
			PathIndex++;   // 到这个路点了，下一个
			return;
		}
	}

	FVector Direction = ComputeMoveDirection(MoveTarget);
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	// 带扫掠地移动：撞上东西就停住（绝不穿墙），下一帧重新寻路绕开
	const FVector Step = Direction.GetSafeNormal() * MoveSpeed * DeltaSeconds;
	FHitResult Hit;
	AddActorWorldOffset(Step, true, &Hit);
	if (Hit.bBlockingHit)
	{
		PathPoints.Reset();
		PathIndex = 0;
		RepathCooldown = 0.0f;
	}
}

bool AMonster::BuildPath(const FVector& Goal)
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!NavSystem)
	{
		if (!bLoggedMovementMode)
		{
			bLoggedMovementMode = true;
			UE_LOG(LogLastDay, Warning, TEXT("[Monster] 场景里没有导航系统，改用避障转向（不会穿墙，但可能绕不过凹形障碍）"));
		}
		return false;
	}

	const ANavigationData* NavData = NavSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
	if (!NavData)
	{
		if (!bLoggedMovementMode)
		{
			bLoggedMovementMode = true;
			UE_LOG(LogLastDay, Warning, TEXT("[Monster] 关卡里没有导航网格（缺 NavMeshBoundsVolume），改用避障转向（不会穿墙，但可能绕不过凹形障碍）"));
		}
		return false;
	}

	const FVector QueryExtent(300.0f, 300.0f, 500.0f);
	FNavLocation ProjectedStart;
	FNavLocation ProjectedGoal;
	if (!NavSystem->ProjectPointToNavigation(GetActorLocation(), ProjectedStart, QueryExtent) ||
		!NavSystem->ProjectPointToNavigation(Goal, ProjectedGoal, QueryExtent))
	{
		return false;
	}

	FPathFindingQuery Query(this, *NavData, ProjectedStart.Location, ProjectedGoal.Location);
	Query.SetAllowPartialPaths(true);

	const FPathFindingResult Result = NavSystem->FindPathSync(Query);
	if (!Result.IsSuccessful() || !Result.Path.IsValid())
	{
		return false;
	}

	PathPoints.Reset();
	for (const FNavPathPoint& PathPoint : Result.Path->GetPathPoints())
	{
		PathPoints.Add(PathPoint.Location);
	}

	// 第一个点是起点，跳过
	PathIndex = (PathPoints.Num() > 1) ? 1 : 0;
	return PathPoints.Num() > 0;
}

FVector AMonster::ComputeMoveDirection(const FVector& Goal) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return FVector::ZeroVector;
	}

	const FVector Start = GetActorLocation();
	FVector Desired = Goal - Start;
	Desired.Z = 0.0f;
	Desired = Desired.GetSafeNormal();
	if (Desired.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const float ProbeDistance = 200.0f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MonsterProbe), false, this);
	if (CurrentTarget)
	{
		Params.AddIgnoredActor(CurrentTarget);
	}

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, Start + Desired * ProbeDistance, ECC_Visibility, Params))
	{
		return Desired;   // 前方畅通，直走
	}

	// 前方被挡：左右各试几个角度，挑第一个畅通的方向（简易避障）
	static const float CandidateAngles[] = { 45.0f, -45.0f, 90.0f, -90.0f, 135.0f, -135.0f };
	for (const float Angle : CandidateAngles)
	{
		const FVector Candidate = Desired.RotateAngleAxis(Angle, FVector::UpVector);
		if (!World->LineTraceSingleByChannel(Hit, Start, Start + Candidate * ProbeDistance, ECC_Visibility, Params))
		{
			return Candidate;
		}
	}

	// 全被挡住：顶着走（扫掠会挡住它，不会穿过去）
	return Desired;
}

// =====================================================
// 瞄准与开火
// =====================================================

void AMonster::UpdateAim(float DeltaSeconds)
{
	if (!Weapon || DeltaSeconds <= 0.0f)
	{
		return;
	}

	// 没有目标就停在当前角度
	FRotator Desired = Weapon->GetBarrelRotation();

	if (CurrentTarget)
	{
		const FVector WorldDir = (CurrentTarget->GetActorLocation() - Weapon->GetActorLocation()).GetSafeNormal();
		if (!WorldDir.IsNearlyZero())
		{
			// 换算成相对自己（挂载父级）的方向
			const FVector LocalDir = GetActorTransform().InverseTransformVectorNoScale(WorldDir);
			Desired = LocalDir.Rotation();
			Desired.Roll = 0.0f;
		}
	}

	const float MaxStep = FMath::Max(RotationSpeed, 0.0f) * DeltaSeconds;
	const FRotator Current = Weapon->GetBarrelRotation();
	FRotator NewRelative = Current;
	NewRelative.Yaw = Current.Yaw + FMath::Clamp(FMath::FindDeltaAngleDegrees(Current.Yaw, Desired.Yaw), -MaxStep, MaxStep);
	NewRelative.Pitch = Current.Pitch + FMath::Clamp(Desired.Pitch - Current.Pitch, -MaxStep, MaxStep);
	NewRelative.Roll = 0.0f;

	Weapon->SetBarrelRotation(NewRelative);
}

void AMonster::UpdateFiring()
{
	if (!Weapon || !CurrentTarget)
	{
		return;
	}

	const FVector Dir = (CurrentTarget->GetActorLocation() - Weapon->GetActorLocation()).GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return;
	}

	const float Dot = FVector::DotProduct(Weapon->GetActorRotation().Vector(), Dir);
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
	if (Angle > AimTolerance)
	{
		return;
	}

	if (Weapon->TryFire() && !bLoggedFirstShot)
	{
		bLoggedFirstShot = true;
		UE_LOG(LogLastDay, Log, TEXT("[Monster] %s 首次开火，目标 %s"), *GetName(), *GetNameSafe(CurrentTarget));
	}
}
