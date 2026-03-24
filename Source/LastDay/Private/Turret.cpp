#include "Turret.h"
#include "TurretSocketComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

ATurret::ATurret()
{
    PrimaryActorTick.bCanEverTick = true;
    detectionRadius = 1000.0f;
    fireCooldown = 0.5f;

    // 创建静态网格体组件，并设为根组件
    cylinderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CylinderMesh"));
    RootComponent = cylinderMesh;

    // 加载引擎内置的圆柱体网格体
    static ConstructorHelpers::FObjectFinder<UStaticMesh> cylinderMeshAsset(
        TEXT("/Engine/BasicShapes/Cylinder")
    );

    if (cylinderMeshAsset.Succeeded())
    {
        cylinderMesh->SetStaticMesh(cylinderMeshAsset.Object);
    }
    else
    {
        // 如果加载失败，可以在日志中输出警告
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Fire!"));
    }

    UTurretSocketComponent socket
    AddSocket()
}

void ATurret::BeginPlay()
{
    Super::BeginPlay();
    // 自动收集所有附加的发射口组件
    GetComponents<UTurretSocketComponent>(sockets);
}

void ATurret::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // 每帧检测敌人并让发射口尝试开火
    UpdateDetection();
    for (UTurretSocketComponent *socket : sockets)
    {
        if (socket)
        {
            socket->TryFire(DeltaTime);
        }
    }
}

void ATurret::UpdateDetection()
{
    // 简单球形检测，获取范围内的敌人
    TArray<FOverlapResult> Overlaps;
    FCollisionShape Shape = FCollisionShape::MakeSphere(detectionRadius);
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

    detectedEnemies.Empty();
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor *Actor = Overlap.GetActor();
        if (Actor && Actor->IsA<ATurret>()) // 过滤敌人类型
        {
            detectedEnemies.Add(Actor);
        }
    }
}

void ATurret::AddSocket(UTurretSocketComponent *socket)
{
    if (socket && !sockets.Contains(socket))
    {
        sockets.Add(socket);
    }
}