// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Unit.generated.h"

class UBoxComponent;

/** 阵营：玩家侧（建筑、炮塔）和怪物侧，用来判敌我 */
UENUM(BlueprintType)
enum class EUnitTeam : uint8
{
	Player  UMETA(DisplayName = "玩家"),
	Monster UMETA(DisplayName = "怪物")
};

/**
 * 单位基类：有血量、护甲、队伍。
 *
 * 受伤统一走引擎标准伤害流程：外面的武器/子弹/敌人用
 * UGameplayStatics::ApplyDamage / ApplyPointDamage / ApplyRadialDamage 发起伤害，
 * 最终都会落到这里的 TakeDamage 上，不再自定义 Hitted 之类的接口。
 */
UCLASS()
class LASTDAY_API AUnit : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AUnit();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** 标准受击入口：ApplyDamage 系列接口最终都会调用到这里 */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	uint64_t GetTeam() const;
	void SetTeam(uint64_t team);

	/** 当前血量 */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Actor 结束播放（含被 Destroy 销毁）时调用，用于在单位阵亡时播放爆炸
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 单位的受击体（根组件）。
	 * 没有模型的单位靠它接收子弹碰撞与炮塔检测；
	 * 自带模型的子类（如 ATurret）可以关掉它、改用模型自身的碰撞，让碰撞体贴合模型。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionComponent;

	/** 当前血量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float Health;

	/** 最大血量：出场时按这个值补满 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth;

	/** 护甲：每次受击固定减免这么多伤害（打不穿也至少掉 1 点） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float Armor;

private:
	uint64_t Id;
	std::string Name;
	uint64_t Team;
};
