// Copyright Epic Games, Inc. All Rights Reserved.

#include "DemoEnemy.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "LastDayGameState.h"
#include "Monster.h"
#include "Unit.h"
#include "EngineUtils.h"
#include "LastDayVisuals.h"

// =====================================================
// ADemoEnemy
// =====================================================

ADemoEnemy::ADemoEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// G2-2：用 AI 控制器驱动寻路（关卡里需要有 NavMeshBoundsVolume）
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(30.0f, 30.0f);

	// G1-1：红色球体外观
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionProfileName(TEXT("NoCollision"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(SphereMesh.Object);
		BodyMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.6f));
	}
}

void ADemoEnemy::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	// 统一配色：敌人 = 紫色
	ApplyLastDayColor(BodyMesh, LastDayColors::Enemy);

	AcquireTarget();
}

void ADemoEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsValid(CurrentTarget))
	{
		AcquireTarget();
	}

	UpdateCombat(DeltaSeconds);
}

void ADemoEnemy::AcquireTarget()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		CurrentTarget = nullptr;
		return;
	}

	// G2-1：优先挑最近的建筑/炮塔（它们都派生自 AUnit）
	AActor* BestTarget = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (TActorIterator<AUnit> It(World); It; ++It)
	{
		AUnit* Unit = *It;
		if (!IsValid(Unit))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Unit->GetActorLocation(), GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Unit;
		}
	}

	// G5-2：建筑都没了就改打玩家
	if (!BestTarget)
	{
		BestTarget = UGameplayStatics::GetPlayerPawn(World, 0);
	}

	CurrentTarget = BestTarget;
}

void ADemoEnemy::UpdateCombat(float DeltaSeconds)
{
	if (AttackCooldown > 0.0f)
	{
		AttackCooldown -= DeltaSeconds;
	}
	if (RepathCooldown > 0.0f)
	{
		RepathCooldown -= DeltaSeconds;
	}

	AAIController* AI = Cast<AAIController>(GetController());
	if (!IsValid(CurrentTarget) || !AI)
	{
		return;
	}

	const float Dist = FVector::Dist(CurrentTarget->GetActorLocation(), GetActorLocation());

	if (Dist <= AttackRange)
	{
		// G3-1：进入攻击范围 → 停下来按间隔打
		GetCharacterMovement()->StopMovementImmediately();

		if (AttackCooldown <= 0.0f)
		{
			// 官方伤害接口：近战伤害
			UGameplayStatics::ApplyDamage(CurrentTarget, AttackDamage, GetController(), this, UDamageType::StaticClass());
			AttackCooldown = AttackInterval;
		}
	}
	else if (RepathCooldown <= 0.0f)
	{
		// G2-2：持续靠近（留一点余量，避免贴着目标抖动）
		AI->MoveToActor(CurrentTarget, FMath::Max(50.0f, AttackRange * 0.8f));
		RepathCooldown = 0.5f;
	}
}

float ADemoEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Health <= 0.0f)
	{
		return 0.0f;
	}

	// 让基类先按伤害类型算出实际伤害
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	Health -= ActualDamage;

	if (Health <= 0.0f)
	{
		// G4-2：死亡时告知波次系统
		if (ALastDayGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ALastDayGameState>() : nullptr)
		{
			GameState->NotifyEnemyKilled();
		}

		Destroy();
	}

	return ActualDamage;
}

// =====================================================
// AEnemySpawner
// =====================================================

AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	MonsterClass = AMonster::StaticClass();
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!MonsterClass)
	{
		MonsterClass = AMonster::StaticClass();
	}

	BuildSpawnPoints();

	// H2-2：监听阶段切换
	if (ALastDayGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ALastDayGameState>() : nullptr)
	{
		GameState->OnDayPhaseChanged.AddDynamic(this, &AEnemySpawner::HandleDayPhaseChanged);
	}
}

void AEnemySpawner::BuildSpawnPoints()
{
	SpawnPoints.Reset();

	const int32 Count = FMath::Max(1, SpawnPointCount);

	// 以玩家为中心摆一圈刷新点：刷怪器本身可能摆在原点，而玩家的基地在别处，
	// 不以玩家为中心的话怪物会刷得太远、够不着基地（它们没有目标就不动）。
	FVector Center = GetActorLocation();
	if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Center = PlayerPawn->GetActorLocation();
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = (2.0f * PI * Index) / Count;
		SpawnPoints.Add(Center + FVector(FMath::Cos(Angle) * SpawnRadius, FMath::Sin(Angle) * SpawnRadius, 100.0f));
	}
}

void AEnemySpawner::HandleDayPhaseChanged(EDayPhase NewPhase)
{
	if (NewPhase != EDayPhase::Night)
	{
		return;
	}

	ALastDayGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ALastDayGameState>() : nullptr;
	WaveTotal = GameState ? GameState->PendingSpawns : 5;
	SpawnedThisWave = 0;

	// 入夜时按玩家当前位置重新摆一遍刷新点
	BuildSpawnPoints();

	if (WaveTotal <= 0 || SpawnPoints.Num() == 0)
	{
		return;
	}

	// H2-3：每隔 1-2 秒生成一只，避免一次性卡顿
	const float Interval = FMath::FRandRange(FMath::Min(MinSpawnInterval, MaxSpawnInterval), FMath::Max(MinSpawnInterval, MaxSpawnInterval));
	GetWorldTimerManager().SetTimer(SpawnTimer, this, &AEnemySpawner::SpawnOne, Interval, true, 0.0f);
}

void AEnemySpawner::SpawnOne()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (SpawnedThisWave >= WaveTotal || !MonsterClass || SpawnPoints.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimer);
		return;
	}

	const FVector BasePoint = SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)];
	const FVector SpawnLocation = BasePoint + FVector(FMath::FRandRange(-150.0f, 150.0f), FMath::FRandRange(-150.0f, 150.0f), 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (World->SpawnActor<AMonster>(MonsterClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams))
	{
		SpawnedThisWave++;

		// H2-4：告诉 GameState 场上多了一个敌人
		if (ALastDayGameState* GameState = World->GetGameState<ALastDayGameState>())
		{
			GameState->NotifyEnemySpawned();
		}
	}
}
