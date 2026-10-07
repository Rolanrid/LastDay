// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "DemoTypes.h"
#include "LastDayGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDayPhaseChanged, EDayPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStarted, int32, NewWave);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrystalChanged, int32, NewAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyCountChanged, int32, Remaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameOver, bool, bVictory);

/**
 * Demo 的全局状态：阶段时序（H1）、波次与敌人计数（H2/H4）、玩家源晶（D2-3）。
 * 由 ALastDayGameMode 在构造函数里指定为 GameStateClass，因此不需要改蓝图。
 */
UCLASS()
class LASTDAY_API ALastDayGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALastDayGameState();

	virtual void Tick(float DeltaSeconds) override;

	// ---------------- 只读状态 ----------------

	/** 当前阶段 */
	UPROPERTY(BlueprintReadOnly, Category = "Demo|State")
	EDayPhase DayPhase = EDayPhase::Landing;

	/** 当前波次（0 = 还没开始） */
	UPROPERTY(BlueprintReadOnly, Category = "Demo|State")
	int32 CurrentWave = 0;

	/** 场上存活的敌人数 */
	UPROPERTY(BlueprintReadOnly, Category = "Demo|State")
	int32 RemainingEnemies = 0;

	/** 本波还没刷出来的敌人数 */
	UPROPERTY(BlueprintReadOnly, Category = "Demo|State")
	int32 PendingSpawns = 0;

	/** 玩家持有的源晶 */
	UPROPERTY(BlueprintReadOnly, Category = "Demo|State")
	int32 PlayerCrystal = 0;

	/** 是否已分出胜负 */
	UPROPERTY(BlueprintReadOnly, Category = "Demo|State")
	bool bGameOver = false;

	// ---------------- 阶段时长配置（H1-3）----------------

	UPROPERTY(EditDefaultsOnly, Category = "Demo|Config")
	float LandingDuration = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Demo|Config")
	float DayDuration = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Demo|Config")
	float DuskDuration = 30.0f;

	/** 夜晚时长（秒）：到点就进入战后，不等敌人清空 */
	UPROPERTY(EditDefaultsOnly, Category = "Demo|Config")
	float NightDuration = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Demo|Config")
	float PostWaveDuration = 60.0f;

	/** 共几波（第 300 波清完即胜利，H1-4） */
	UPROPERTY(EditDefaultsOnly, Category = "Demo|Config")
	int32 MaxWaves = 300;

	/** 屏幕左上角的临时状态文字（J 模块的 HUD 做好后可以关掉） */
	UPROPERTY(EditDefaultsOnly, Category = "Demo|Config")
	bool bShowDebugHUD = true;

	// ---------------- 事件 ----------------

	UPROPERTY(BlueprintAssignable, Category = "Demo|Events")
	FOnDayPhaseChanged OnDayPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Demo|Events")
	FOnWaveStarted OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category = "Demo|Events")
	FOnCrystalChanged OnCrystalChanged;

	UPROPERTY(BlueprintAssignable, Category = "Demo|Events")
	FOnEnemyCountChanged OnEnemyCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Demo|Events")
	FOnGameOver OnGameOver;

	// ---------------- 查询 ----------------

	/** H2-2：波次 1/2/3 → 5/10/15 */
	UFUNCTION(BlueprintPure, Category = "Demo")
	int32 GetEnemyCountForWave(int32 Wave) const;

	UFUNCTION(BlueprintPure, Category = "Demo")
	bool IsNight() const { return DayPhase == EDayPhase::Night; }

	/** HUD 用：本波还差多少敌人（存活 + 未刷出） */
	UFUNCTION(BlueprintPure, Category = "Demo")
	int32 GetRemainingInWave() const { return RemainingEnemies + PendingSpawns; }

	// ---------------- 操作 ----------------

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetDayPhase(EDayPhase NewPhase);

	/** 进入夜晚：波次 +1，重置本波敌人预算 */
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void StartNightWave();

	/** 刷出一个敌人：待刷数 -1、存活数 +1（H2-4） */
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void NotifyEnemySpawned();

	/** 一个敌人死亡：存活数 -1（只更新计数，夜晚由固定时长结束） */
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void NotifyEnemyKilled();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void AddCrystal(int32 Amount);

	/** 消耗源晶，返回是否足够（D2-3） */
	UFUNCTION(BlueprintCallable, Category = "Demo")
	bool ConsumeCrystal(int32 Amount);

	/** I1/I2：结束流程（胜利或失败） */
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void TriggerGameOver(bool bVictory);

protected:
	virtual void BeginPlay() override;

private:
	/** 当前阶段已经过去的时间 */
	float PhaseElapsed = 0.0f;

	/** 按时间推进阶段（夜晚也按固定时长结束） */
	void AdvancePhaseByTime();

	/** 夜晚结束：最后一波打完就胜利，否则进入战后阶段 */
	void EndNight();
};
