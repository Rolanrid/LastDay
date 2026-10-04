#include "Turret.h"  
#include "TurretSocketComponent.h"  
#include "Engine/OverlapResult.h"  
#include "Engine/World.h"  
#include "BaseProjectile.h"
#include "Components/BoxComponent.h" // AUnit 的默认受击体类型（本类里要关掉它）

ATurret::ATurret()  
{  
   PrimaryActorTick.bCanEverTick = true;  
   DetectionRadius = 1000.0f;  
   FireCooldown = 0.5f;  

    // 创建静态网格体组件，挂在 AUnit 的碰撞根上  
   TurretRoot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));  
    TurretRoot->SetupAttachment(RootComponent);  
    // 炮台有模型，直接用模型自身的简单碰撞（立方体），与模型完全贴合；
    // 关掉 AUnit 默认的碰撞盒，避免出现两个重叠的碰撞体  
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);  
    TurretRoot->SetCollisionProfileName(TEXT("BlockAllDynamic"));  
   ProjectileClass = ABaseProjectile::StaticClass();  

   // 加载引擎内置的长方体网格体  
   static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeMeshAsset(  
       TEXT("/Engine/BasicShapes/Cube")  
    );  

   if (cubeMeshAsset.Succeeded()) {  
       TurretRoot->SetStaticMesh(cubeMeshAsset.Object);  
   } else {  
       // 如果加载失败，可以在日志中输出警告  
       GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Find Cube failed!"));  
   }  

   for (int32 i = 0; i < 2; ++i) {  
       FName socketName = *FString::Printf(TEXT("TurretSocket_%d"), i);  
       UTurretSocketComponent* socket = CreateDefaultSubobject<UTurretSocketComponent>(socketName);  
       socket->SetupAttachment(RootComponent);  
       socket->SetRelativeLocation(FVector(10, (i * 40 - 20), 60));  
       // 不旋转：长方体长度轴即组件前向（X），组件前向就是炮口方向，子弹按此方向发射  
       socket->SetRelativeRotation(FRotator::ZeroRotator);  
       // X 为炮管长度（1.0 → 100），Y/Z 为截面粗细（0.1 → 10），与原来外观一致  
       socket->SetRelativeScale3D(FVector(1.0f, 0.2f, 0.2f));  
       Sockets.Add(socket);  
   }  
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
    // 发射口的“找目标 / 转向 / 开火”由 UTurretSocketComponent 自己每帧处理（见 TickComponent），
    // 这里只保留炮塔层的整体检测，供后续 AI、状态提示等使用，避免同一帧重复驱动发射口。
    UpdateDetection();
}

void ATurret::UpdateDetection()
{
    // 简单球形检测，获取范围内的敌人
    TArray<FOverlapResult> overlaps;
    FCollisionShape shape = FCollisionShape::MakeSphere(DetectionRadius);
    FCollisionQueryParams queryParams;
    queryParams.AddIgnoredActor(this);

    GetWorld()->OverlapMultiByChannel(
        overlaps,
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,   // 适合检测角色的通道
        shape,
        queryParams
    );

    DetectedEnemies.Empty();
    for (const FOverlapResult& overlap : overlaps) {
        AActor* actor = overlap.GetActor();
        if (actor && actor->IsA<AUnit>())// && Cast<AUnit>(Actor)->GetTeam() != GetTeam()) // 过滤敌人类型
        {
            if (DetectedEnemies.Find(actor)) {
                continue;
            }
            DetectedEnemies.Add(actor);
            // GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Find!"));
        }
    }
}

void ATurret::AddSocket(UTurretSocketComponent* socket)
{
    if (socket && !Sockets.Contains(socket))
    {
        socket->OwnerTurret = this; // 设置所属炮塔指针
        Sockets.Add(socket);
    }
}
