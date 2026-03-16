#include "Turret.h"
#include "TurretSocketComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

ATurret::ATurret()
{
    PrimaryActorTick.bCanEverTick = true;
    DetectionRadius = 1000.0f;
    FireCooldown = 0.5f;
}

void ATurret::BeginPlay()
{
    Super::BeginPlay();
    // 自动收集所有附加的发射口组件
    GetComponents<UTurretSocketComponent>(Sockets);
}

void ATurret::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // 每帧检测敌人并让发射口尝试开火
    UpdateDetection();
    for (UTurretSocketComponent* Socket : Sockets)
    {
        if (Socket)
        {
            Socket->TryFire(DeltaTime);
        }
    }
}

void ATurret::UpdateDetection()
{
    // 简单球形检测，获取范围内的敌人
    TArray<FOverlapResult> Overlaps;
    FCollisionShape Shape = FCollisionShape::MakeSphere(DetectionRadius);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    GetWorld()->OverlapMultiByChannel(
        Overlaps,
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,   // 适合检测角色的通道
        Shape,
        QueryParams
    );

    DetectedEnemies.Empty();
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Actor = Overlap.GetActor();
        if (Actor && Actor->IsA<AEnemyCharacter>()) // 过滤敌人类型
        {
            DetectedEnemies.Add(Actor);
        }
    }
}

void ATurret::AddSocket(UTurretSocketComponent* Socket)
{
    if (Socket && !Sockets.Contains(Socket))
    {
        Sockets.Add(Socket);
    }
}