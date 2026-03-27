// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TurretSocketComponent.generated.h"

class ABaseProjectile;
class ATurret;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UTurretSocketComponent : public UStaticMeshComponent
{
    GENERATED_BODY()

public:
    UTurretSocketComponent();

    /** 尝试开火，由炮塔每帧调用 */
    void TryFire(float DeltaTime);

    /** 所属炮塔 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    ATurret* ownerTurret;

protected:
    virtual void BeginPlay() override;

    /** 当前锁定目标 */
    UPROPERTY()
    AActor* currentTarget;

    /** 开火冷却计时器 */
    float cooldownRemaining;

    /** 检测前方角度范围（度） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
    float fireAngle;

    /** 最大射程 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
    float fireRange;

    /** 子弹速度（用于预测，此处简化） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
    float projectileSpeed;

    /** 子弹类（可覆盖炮塔的默认类） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
    TSubclassOf<ABaseProjectile> projectileClass;

private:
    /** 检测是否有敌人在前方扇形内 */
    AActor* FindTargetInCone();

    /** 发射子弹 */
    void Fire();
};
