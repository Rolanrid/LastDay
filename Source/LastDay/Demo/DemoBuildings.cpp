// Copyright Epic Games, Inc. All Rights Reserved.

#include "DemoBuildings.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "LastDayGameState.h"
#include "MiningDamageType.h"
#include "EngineUtils.h"
#include "LastDay.h"
#include "LastDayVisuals.h"

// =====================================================
// ASourceCrystal
// =====================================================

ASourceCrystal::ASourceCrystal()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrystalMesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));   // 能被子弹打到

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.2f));   // 60×60×120 的柱状晶矿
	}
}

void ASourceCrystal::BeginPlay()
{
	Super::BeginPlay();

	CurrentAmount = MaxAmount;
	// 统一材质：源晶 = 玻璃
	ApplyLastDayGlassColor(Mesh, LastDayColors::Glass);
}

float ASourceCrystal::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 保持引擎的伤害事件链完整（OnTakeAnyDamage 等）
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 只有采矿伤害（UMiningDamageType）才能采集：普通枪械打矿脉不掉矿
	const bool bMiningDamage = DamageEvent.DamageTypeClass && DamageEvent.DamageTypeClass->IsChildOf(UMiningDamageType::StaticClass());
	if (!bMiningDamage)
	{
		return 0.0f;
	}

	// 对矿脉来说"被打中"等于"被采集"，采集量固定，与子弹伤害无关
	Gather(GatherPerHit);

	return static_cast<float>(GatherPerHit);
}

void ASourceCrystal::Gather(int32 Amount)
{
	if (Amount <= 0 || CurrentAmount <= 0.0f)
	{
		return;
	}

	CurrentAmount = FMath::Max(0.0f, CurrentAmount - Amount);

	// D2-1：采集到的源晶直接进玩家资源
	if (ALastDayGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALastDayGameState>() : nullptr)
	{
		GS->AddCrystal(Amount);
	}

	if (CurrentAmount <= 0.0f)
	{
		Destroy();   // D1-3：矿脉枯竭
	}
}

// =====================================================
// ACollectorBuilding
// =====================================================

ACollectorBuilding::ACollectorBuilding()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CollectorMesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.8f));
	}

	// 有模型就用模型自身的碰撞，关掉 AUnit 的默认盒子（与 ATurret 一致）
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ACollectorBuilding::BeginPlay()
{
	Super::BeginPlay();

	// 统一配色：采集器属于建造物 = 黄色
	ApplyLastDayColor(Mesh, LastDayColors::Construction);

	// D3-1：用定时器每秒产出一次
	GetWorldTimerManager().SetTimer(ProduceTimer, this, &ACollectorBuilding::Produce, FMath::Max(0.1f, ProduceInterval), true);
}

void ACollectorBuilding::Produce()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// D3-2：找 GatherRadius 内最近的一座活跃矿脉
	ASourceCrystal* NearestCrystal = nullptr;
	float NearestDistSq = FMath::Square(GatherRadius);

	for (TActorIterator<ASourceCrystal> It(World); It; ++It)
	{
		ASourceCrystal* Crystal = *It;
		if (!IsValid(Crystal))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Crystal->GetActorLocation(), GetActorLocation());
		if (DistSq <= NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestCrystal = Crystal;
		}
	}

	if (NearestCrystal)
	{
		NearestCrystal->Gather(ProducePerTick);
	}
}
