// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Unit.generated.h"

class UBoxComponent;

UCLASS()
class LASTDAY_API AUnit : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AUnit();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	uint64_t GetTeam() const;
	void SetTeam(uint64_t team);

	virtual void Hitted(uint64_t damage);

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

private:
	uint64_t Id;
	std::string Name;
	uint64_t Team;
	uint64_t Hp;
	uint64_t Armor;
};
