// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Explosion.h"
#include "ExplosionEffect.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * 最简爆炸表现：在指定扩散时长内从 0 均匀膨胀到目标半径，再按淡出时长渐隐，
 * 完全不可见后才销毁的橙红色球体。用引擎白模球 + 一个半透材质在运行时染色。
 * 半径 / 扩散时长 / 淡出时长 / 伤害由工厂函数 SpawnExplosion() 在生成时传入。
 */
UCLASS()
class LASTDAY_API AExplosionEffect : public AExplosion
{
	GENERATED_BODY()

public:
	AExplosionEffect();

	// ---- 以下参数由工厂函数 SpawnExplosion() 在 FinishSpawning 之前写入 ----

	/** 爆炸最终半径（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float Radius;

	/** 扩散时长（秒）：球体在这段时间内从 0 均匀膨胀到 Radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float ExpansionTime;

	/** 淡出时长（秒）：扩散完成后在这段时间内从全不透明渐隐到完全不可见 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float FadeOutTime;

	/** 范围伤害：大于 0 时在 BeginPlay 里按官方接口 UGameplayStatics::ApplyRadialDamage 结算一次；纯表现特效传 0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float Damage;

	/** 爆炸球体使用的半透材质（需要 Color 向量参数 + Opacity 标量参数才能淡出） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosion")
	UMaterialInterface* ExplosionMaterial;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** 爆炸球体（根组件） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* SphereMesh;

private:
	/** 当前半径（cm） */
	float CurrentRadius;

	/** 扩散已经过去的时间（秒） */
	float ExpansionElapsed;

	/** 淡出已经过去的时间（秒） */
	float FadeElapsed;

	/** 运行时创建的动态材质，用来改颜色与透明度 */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* DynamicMaterial;
};

// 工厂函数：在指定位置生成一次爆炸表现。
// Radius 最终半径(cm)、ExpansionTime 扩散时长(s)、FadeOutTime 淡出时长(s)、Damage 范围伤害（0 = 只做表现）
AExplosionEffect* SpawnExplosion(UWorld* World, TSubclassOf<AExplosionEffect> ExplosionClass, FVector Location, float Radius, float ExpansionTime, float FadeOutTime, float Damage);
