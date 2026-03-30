
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseProjectile.generated.h"

// 提前声明组件和类，加快编译速度
class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class AUnit; // 提前声明你的 Unit 类

UCLASS()
class LASTDAY_API ABaseProjectile : public AActor
{
	GENERATED_BODY()

public:
	// 构造函数：设置默认属性
	ABaseProjectile();

protected:
	// 游戏开始或生成时调用
	virtual void BeginPlay() override;

public:
	// 每一帧调用（炮弹一般不需要Tick，运动组件会代劳，所以我关掉了）
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

	// 抛射体运动组件，UE5自带的运动外挂！
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UProjectileMovementComponent* ProjectileMovement;

	// ==========================================
	// 属性与接口
	// ==========================================

	// 子弹的伤害值，默认 10
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Damage;

	// 子弹的最大射程，默认 1000
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxRange;

	// 接口：炮塔调用子弹开火！(传入飞行方向、速度、发射它的塔)
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void FireProjectile(const FVector& ShootDirection, float ShootSpeed, AActor* Shooter);

	// 碰撞绑定的函数：打到实体或环境时触发
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	// 记录一下是谁发射了这颗子弹
	UPROPERTY()
	AActor* MyShooter;
};