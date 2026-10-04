// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "TurretSocketComponent.generated.h"

class ABaseProjectile;
class ATurret;

/**
 * 炮塔发射口。
 *
 * 组件自身每帧 Tick：刷新目标 → 朝目标限速转动 → 冷却结束后对准目标开火。
 * 转动范围、转动速度、瞄准容差都可以在细节面板里调。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LASTDAY_API UTurretSocketComponent : public UStaticMeshComponent
{
    GENERATED_BODY()

public:
    UTurretSocketComponent();

    /**
     * 发射口的主更新入口：刷新目标 → 转向 → 冷却与开火判定。
     * 组件自己在 TickComponent 里每帧调用，一般不需要外部再驱动。
     * @return 本帧是否真的开火了
     */
    bool TryFire(float DeltaTime);

    /** 当前锁定的目标（可能为空） */
    UFUNCTION(BlueprintPure, Category = "Turret|Socket")
    AActor* GetCurrentTarget() const;

    /** 当前炮口方向与目标方向的夹角（度）；没有目标时返回 180 */
    UFUNCTION(BlueprintPure, Category = "Turret|Socket")
    float GetAimAngleToTarget() const;

    /** 所属炮塔 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    ATurret* OwnerTurret;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** 当前锁定目标 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Socket")
    TObjectPtr<AActor> CurrentTarget;

    /** 开火冷却计时器 */
    float CooldownRemaining;

    /** 检测角度范围（度）：相对炮塔底座前向的半角 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
    float FireAngle;

    /** 最大射程 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
    float FireRange;

    /** 子弹速度（用于预测，此处简化） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
    float ProjectileSpeed;

    /** 子弹类（可覆盖炮塔的默认类） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
    TSubclassOf<ABaseProjectile> ProjectileClass;

    // ---------------- 转向 / 瞄准 ----------------

    /** 转向速度（度/秒），越大跟得越紧 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket|Aim", meta = (ClampMin = "0.0"))
    float RotationSpeed;

    /** 是否限制发射口相对炮塔底座的转动范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket|Aim")
    bool bLimitRotation;

    /** 底座左右最大偏航角（度，左右对称） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket|Aim", meta = (EditCondition = "bLimitRotation", ClampMin = "0.0", ClampMax = "180.0"))
    float MaxYawAngle;

    /** 俯仰下限（度，相对底座） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket|Aim", meta = (EditCondition = "bLimitRotation", ClampMin = "-89.0", ClampMax = "89.0"))
    float MinPitchAngle;

    /** 俯仰上限（度，相对底座） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket|Aim", meta = (EditCondition = "bLimitRotation", ClampMin = "-89.0", ClampMax = "89.0"))
    float MaxPitchAngle;

    /** 瞄准容差（度）：炮口与目标夹角小于该值才开火，避免边转边乱射 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket|Aim", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float AimTolerance;

    /** 是否绘制瞄准调试线 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket|Aim")
    bool bDrawDebugAim;

private:
    /** 刷新当前目标：旧目标还合格就继续锁定，否则重新搜索 */
    void UpdateTarget();

    /** 朝当前目标转动（按 RotationSpeed 限速） */
    void UpdateAim(float DeltaTime);

    /** 冷却计时，并在对准目标时开火。返回本帧是否开火 */
    bool UpdateFiring(float DeltaTime);

    /** 在检测范围内挑出最近的一个 Unit 作为目标 */
    AActor* FindTargetInCone();

    /** 判断某个 Actor 是否能作为目标（类型 / 距离 / 角度都满足） */
    bool IsTargetCandidate(const AActor* Actor) const;

    /** 检测与限角用的基准前向：优先取炮塔底座（父组件）的前向 */
    FVector GetBaseForwardVector() const;

    /** 发射子弹 */
    void Fire();
};
