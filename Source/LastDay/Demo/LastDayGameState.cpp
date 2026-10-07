// Copyright Epic Games, Inc. All Rights Reserved.

#include "LastDayGameState.h"
#include "LastDay.h"

ALastDayGameState::ALastDayGameState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALastDayGameState::BeginPlay()
{
	Super::BeginPlay();

	// 开局进入"着陆"阶段
	SetDayPhase(EDayPhase::Landing);
}

void ALastDayGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 临时状态显示（H4-2 的占位，等 J 模块的 HUD 做好后可关掉）
	if (bShowDebugHUD && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1001, 0.0f, FColor::White,
			FString::Printf(TEXT("阶段:%d  波次:%d/%d  剩余敌人:%d  源晶:%d%s"),
				(int32)DayPhase, CurrentWave, MaxWaves, GetRemainingInWave(), PlayerCrystal,
				bGameOver ? TEXT("  [游戏结束]") : TEXT("")));
	}

	if (bGameOver)
	{
		return;
	}

	PhaseElapsed += DeltaSeconds;
	AdvancePhaseByTime();
}

void ALastDayGameState::AdvancePhaseByTime()
{
	switch (DayPhase)
	{
	case EDayPhase::Landing:
		if (PhaseElapsed >= LandingDuration)
		{
			SetDayPhase(EDayPhase::Day);
		}
		break;

	case EDayPhase::Day:
		if (PhaseElapsed >= DayDuration)
		{
			SetDayPhase(EDayPhase::Dusk);
		}
		break;

	case EDayPhase::Dusk:
		if (PhaseElapsed >= DuskDuration)
		{
			StartNightWave();
		}
		break;

	case EDayPhase::Night:
		// 夜晚按固定时长结束，不等场上敌人清空
		if (PhaseElapsed >= NightDuration)
		{
			EndNight();
		}
		break;

	case EDayPhase::PostWave:
		if (PhaseElapsed >= PostWaveDuration)
		{
			SetDayPhase(EDayPhase::Day);
		}
		break;
	}
}

void ALastDayGameState::EndNight()
{
	// H1-4：最后一波打完即胜利，否则进入战后阶段
	if (CurrentWave >= MaxWaves)
	{
		TriggerGameOver(true);
	}
	else
	{
		SetDayPhase(EDayPhase::PostWave);
	}
}

void ALastDayGameState::SetDayPhase(EDayPhase NewPhase)
{
	if (DayPhase == NewPhase)
	{
		return;
	}

	DayPhase = NewPhase;
	PhaseElapsed = 0.0f;

	UE_LOG(LogLastDay, Log, TEXT("[GameState] 阶段切换 -> %d"), (int32)NewPhase);
	OnDayPhaseChanged.Broadcast(NewPhase);
}

void ALastDayGameState::StartNightWave()
{
	CurrentWave++;
	RemainingEnemies = 0;
	PendingSpawns = GetEnemyCountForWave(CurrentWave);

	UE_LOG(LogLastDay, Log, TEXT("[GameState] 第 %d 波开始，敌人 %d 只"), CurrentWave, PendingSpawns);

	OnWaveStarted.Broadcast(CurrentWave);
	OnEnemyCountChanged.Broadcast(GetRemainingInWave());

	SetDayPhase(EDayPhase::Night);
}

void ALastDayGameState::NotifyEnemySpawned()
{
	PendingSpawns = FMath::Max(0, PendingSpawns - 1);
	RemainingEnemies++;

	OnEnemyCountChanged.Broadcast(GetRemainingInWave());
}

void ALastDayGameState::NotifyEnemyKilled()
{
	RemainingEnemies = FMath::Max(0, RemainingEnemies - 1);

	OnEnemyCountChanged.Broadcast(GetRemainingInWave());
}

void ALastDayGameState::AddCrystal(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	PlayerCrystal += Amount;
	OnCrystalChanged.Broadcast(PlayerCrystal);
}

bool ALastDayGameState::ConsumeCrystal(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}

	if (PlayerCrystal < Amount)
	{
		return false;
	}

	PlayerCrystal -= Amount;
	OnCrystalChanged.Broadcast(PlayerCrystal);
	return true;
}

void ALastDayGameState::TriggerGameOver(bool bVictory)
{
	if (bGameOver)
	{
		return;
	}

	bGameOver = true;
	UE_LOG(LogLastDay, Log, TEXT("[GameState] 游戏结束：%s"), bVictory ? TEXT("胜利") : TEXT("失败"));
	OnGameOver.Broadcast(bVictory);
}

int32 ALastDayGameState::GetEnemyCountForWave(int32 Wave) const
{
	switch (Wave)
	{
	case 1:  return 5;
	case 2:  return 10;
	case 3:  return 15;
	default: return 5 + FMath::Max(0, Wave - 1) * 5;
	}
}
