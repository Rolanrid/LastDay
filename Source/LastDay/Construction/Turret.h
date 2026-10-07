// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Unit.h"
#include "Turret.generated.h"

class ACannon;
class ACannonBall;
class UStaticMeshComponent;

/**
 * 炮塔。
 *
 * 负责索敌（XY 平面算距离与角度，Z 轴单独判断高度范围）、把炮口转向目标、对准后开火。
 * 炮口自己（ACannon）只负责伤害值、开火间隔、发射子弹这三件事。
 */
UCLASS()
class LASTDAY_API ATurret : public AUnit
{
    GENERATED_BODY()

public:
    ATurret();

    /** 炮口类（默认 ACannon，可以换成蓝图子类） */
    UPROPERTY(EditDefaultsOnly, Category = "Turret|Weapon")
    TSubclassOf<ACannon> CannonClass;

    /** 炮塔默认子弹类：下发给没有单独指定子弹的炮口 */
    UPROPERTY(EditDefaultsOnly, Category = "Turret|Weapon")
    TSubclassOf<class ACannonBall> ProjectileClass;

    /** 当前锁定的目标（可能为空） */
    UFUNCTION(BlueprintPure, Category = "Turret")
    AActor* GetCurrentTarget() const { return CurrentTarget; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** 炮塔主体（TODO 应该是一个模型，这里是白模） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* TurretRoot;

    /** 所有炮口（武器 Actor） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
    TArray<TObjectPtr<ACannon>> Cannons;

    /** 添加炮口（编辑器或运行时都可调用） */
    UFUNCTION(BlueprintCallable, Category = "Turret")
    void AddCannon(ACannon* Cannon);

    // ---------------- 索敌 ----------------

    /** 水平检测半径（cm）：只在 XY 平面上量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Detection", meta = (ClampMin = "0.0"))
    float DetectionRadius;

    /** 水平检测半角（度）：目标与炮塔前向在 XY 平面上的夹角上限 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Detection", meta = (ClampMin = "0.0", ClampMax = "180.0"))
    float DetectionAngle;

    /** Z 轴检测下限（cm）：目标相对炮塔的高度差 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Detection")
    float MinHeightOffset;

    /** Z 轴检测上限（cm） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Detection")
    float MaxHeightOffset;

    // ---------------- 瞄准 ----------------

    /** 炮口转向速度（度/秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aim", meta = (ClampMin = "0.0"))
    float RotationSpeed;

    /** 是否限制炮口相对炮塔底座的转动范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aim")
    bool bLimitAimRotation;

    /** 底座左右最大偏航角（度，左右对称） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aim", meta = (EditCondition = "bLimitAimRotation", ClampMin = "0.0", ClampMax = "180.0"))
    float MaxYawAngle;

    /** 俯仰下限（度，相对底座） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aim", meta = (EditCondition = "bLimitAimRotation", ClampMin = "-89.0", ClampMax = "89.0"))
    float MinPitchAngle;

    /** 俯仰上限（度，相对底座） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aim", meta = (EditCondition = "bLimitAimRotation", ClampMin = "-89.0", ClampMax = "89.0"))
    float MaxPitchAngle;

    /** 瞄准容差（度）：炮口与目标夹角小于该值才开火 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aim", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float AimTolerance;

    /** 是否绘制索敌 / 瞄准的调试线 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Debug")
    bool bDrawDebugAim;

private:
    /** 当前锁定目标 */
    UPROPERTY()
    TObjectPtr<AActor> CurrentTarget;

    /** 生成默认的两个炮口（左右各一个） */
    void SpawnDefaultCannons();

    /** 刷新目标：旧目标还合格就继续锁定，否则重新搜索 */
    void UpdateTarget();

    /** 把每个炮口朝目标转过去（限速 + 限角） */
    void UpdateAim(float DeltaTime);

    /** 炮口对准目标后让它开火（开火间隔由炮口自己管） */
    void UpdateFiring();

    /** 目标筛选：类型、XY 距离与角度、Z 高度范围 */
    bool IsTargetCandidate(const AActor* Actor) const;

    /** 在检测范围内挑最近的合格目标 */
    AActor* FindTarget();

    /** 炮口当前朝向与目标方向的夹角（度）；没有目标时返回 180 */
    float GetAimAngleToTarget(const ACannon& Cannon) const;

    /** 画索敌范围与当前目标（bDrawDebugAim 打开时） */
    void DrawDebugAim();
};
