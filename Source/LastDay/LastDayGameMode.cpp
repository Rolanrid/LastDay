// Copyright Epic Games, Inc. All Rights Reserved.

#include "LastDayGameMode.h"
#include "DemoBuildings.h"
#include "DemoEnemy.h"
#include "DayNightCycleManager.h"
#include "LastDayGameState.h"
#include "Construction/Turret.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"

ALastDayGameMode::ALastDayGameMode()
{
	// 使用 Demo 的 GameState（阶段/波次/资源都在里面），蓝图子类不覆盖的话就会生效
	GameStateClass = ALastDayGameState::StaticClass();
}

void ALastDayGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// A2/H3：关卡里没有昼夜光照管理器就补一个（它会找到/创建 DirectionalLight 等）
	if (!TActorIterator<ADayNightCycleManager>(World))
	{
		World->SpawnActor<ADayNightCycleManager>();
	}

	// H2-1：关卡里没有刷怪点就补一个（自带一圈刷新点）
	if (!TActorIterator<AEnemySpawner>(World))
	{
		World->SpawnActor<AEnemySpawner>();
	}

	SpawnInitialActors();
}

void ALastDayGameMode::SpawnInitialActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 以玩家出生点为基准做兜底摆放；没有 PlayerStart 就用原点
	FVector BaseLocation = FVector(0.0f, 0.0f, 100.0f);
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		BaseLocation = It->GetActorLocation();
		break;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// D1-4：矿脉
	if (bSpawnDemoCrystals && !TActorIterator<ASourceCrystal>(World))
	{
		for (int32 Index = 0; Index < FMath::Max(1, DemoCrystalCount); ++Index)
		{
			const float Angle = (2.0f * PI * Index) / FMath::Max(1, DemoCrystalCount);
			const FVector Location = BaseLocation + FVector(FMath::Cos(Angle) * DemoCrystalRadius, FMath::Sin(Angle) * DemoCrystalRadius, 0.0f);
			World->SpawnActor<ASourceCrystal>(ASourceCrystal::StaticClass(), Location + FVector(0.0f, 0.0f, 60.0f), FRotator::ZeroRotator, SpawnParams);
		}
	}

	// E3-6：初始炮塔
	if (bSpawnInitialTurrets && !TActorIterator<ATurret>(World))
	{
		for (int32 Index = 0; Index < FMath::Max(1, InitialTurretCount); ++Index)
		{
			const float Angle = (2.0f * PI * Index) / FMath::Max(1, InitialTurretCount);
			const FVector Location = BaseLocation + FVector(FMath::Cos(Angle) * InitialTurretRadius, FMath::Sin(Angle) * InitialTurretRadius, 60.0f);
			World->SpawnActor<ATurret>(ATurret::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
		}
	}
}
