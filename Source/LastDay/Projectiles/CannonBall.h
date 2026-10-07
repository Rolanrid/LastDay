// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.h"
#include "CannonBall.generated.h"

// 提前声明组件和类，加快编译速度
class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;

/**
 * 炮塔的炮弹（原来的 ABaseProjectile）。
 * 伤害值由开火的武器（ACannon）在生成时写入，命中时用官方伤害接口
 * UGameplayStatics::ApplyPointDamage 结算，子弹之间靠碰撞预设互相忽略。
 */
UCLASS()
class LASTDAY_API ACannonBall : public AProjectile
{
	GENERATED_BODY()

public:
	// 构造函数：设置默认属性
	ACannonBall();

protected:
	// 游戏开始或生成时调用
	virtual void BeginPlay() override;

public:
	// 每一帧调用（炮弹一般不需要 Tick，运动组件会代劳，所以关掉了）
	virtual void Tick(float DeltaTime) override;

	// ==========================================
	// 组件声明
	// ==========================================

	// 碰撞球组件（透明盒子），用来检测有没有打中东西
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionComp;

	// 炮弹的外观模型
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ProjectileMesh;

	// 抛射体运动组件，UE5 自带的运动外挂
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UProjectileMovementComponent* ProjectileMovement;

	// ==========================================
	// 属性与接口
	// ==========================================

	// 伤害值：由开火的武器在生成时写入，命中时交给官方伤害接口结算
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Damage;

	// 炮弹的最大射程，默认 1000
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxRange;

	// 记录一下是谁发射了这颗炮弹（由工厂函数 FireProjectile 在生成时写入，用来算伤害归属）
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	AActor* MyShooter;

	// 碰撞绑定的函数：打到实体或环境时触发
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};

// 工厂函数：延迟生成炮弹，并在 FinishSpawning() 之前完成初始化（初速度、伤害、忽略发射者）
ACannonBall* FireProjectile(UWorld* World, TSubclassOf<ACannonBall> ProjectileClass, AActor* Shooter, FVector Location, FRotator Rotation, float Speed, float Damage);
