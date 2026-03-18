// Fill out your copyright notice in the Description page of Project Settings.

//#pragma once
//
//#include "CoreMinimal.h"
//#include "Unit.h"
//#include "Turret.generated.h"
//
///**
// * 
// */
//UCLASS()
//class LASTDAY_API ATurret : public AUnit
//{
//    GENERATED_BODY()
//
//public:
//    ATurret();
//
//protected:
//    virtual void BeginPlay() override;
//    virtual void Tick(float DeltaTime) override;
//
//    /** 所有发射口组件 */
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
//    TArray<UTurretSocketComponent*> Sockets;
//
//    /** 添加发射口组件（可在编辑器或运行时调用） */
//    UFUNCTION(BlueprintCallable, Category = "Turret")
//    void AddSocket(UTurretSocketComponent* Socket);
//
//    /** 敌人检测范围（用于发射口独立检测） */
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Detection")
//    float DetectionRadius;
//
//    /** 子弹类 */
//    UPROPERTY(EditDefaultsOnly, Category = "Turret|Combat")
//    TSubclassOf<class ABaseProjectile> ProjectileClass;
//
//    /** 开火间隔（秒） */
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Combat")
//    float FireCooldown;
//
//private:
//    /** 所有发射口共享的敌人集合（可选，用于优化检测） */
//    TArray<AActor*> DetectedEnemies;
//};
