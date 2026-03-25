// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Unit.h"
#include "TurretSocketComponent.h" // Ensure this include is present and correct
#include "Turret.generated.h"

UCLASS()
class LASTDAY_API ATurret : public AUnit
{
    GENERATED_BODY()

public:
	ATurret();

	/** 子弹类 */
	UPROPERTY(EditDefaultsOnly, Category = "Turret|Combat")
	TSubclassOf<class ABaseProjectile> projectileClass;

	/** 开火间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Combat")
	float fireCooldown;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* turretRoot; // TODO 应该是一个模型，这里是白模

	/** 所有发射口组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
	TArray<UTurretSocketComponent*> sockets;

	/** 添加发射口组件（可在编辑器或运行时调用） */
	UFUNCTION(BlueprintCallable, Category = "Turret")
	void AddSocket(UTurretSocketComponent *Socket);

	/** 敌人检测范围（用于发射口独立检测） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Detection")
	float detectionRadius;

private:
	// 根组件（用默认子对象方式创建）
	UPROPERTY(VisibleAnywhere)
	USceneComponent* rootScene;

	/** 所有发射口共享的敌人集合（可选，用于优化检测） */
	TArray<AActor*> detectedEnemies;

	void UpdateDetection();
};
