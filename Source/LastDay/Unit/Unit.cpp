// Fill out your copyright notice in the Description page of Project Settings.


#include "Unit.h"
#include "Components/BoxComponent.h"
#include "Engine/DamageEvents.h"
#include "ExplosionEffect.h"
#include "MiningDamageType.h"

// Sets default values
AUnit::AUnit()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 属性默认值
	MaxHealth = 100.0f;
	Health = MaxHealth;
	Armor = 5.0f;
	Team = 0;

	// 受击体：尺寸与项目里常用的白模方块（引擎 Cube，100³）一致，保证任何单位都能被命中
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("UnitCollision"));
	CollisionComponent->InitBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	// 必须是“阻塞”预设，子弹的 OnHit 才会触发；纯 Overlap 预设只会触发 OnComponentBeginOverlap
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);
	RootComponent = CollisionComponent;
}

uint64_t AUnit::GetTeam() const
{
	return Team;
}

void AUnit::SetTeam(uint64_t in_team)
{
	Team = in_team;
}

// Called when the game starts or when spawned
void AUnit::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;   // 满血出场
}

void AUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 单位被销毁（阵亡）时放一个爆炸表现；关卡切换/退出编辑器时不放
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		if (UWorld* World = GetWorld())
		{
			// 半径 150cm、扩散 1.0s、淡出 1.0s；这里只做表现，伤害传 0
			SpawnExplosion(World, AExplosionEffect::StaticClass(), GetActorLocation(), 150.0f, 1.0f, 1.0f, 0.0f);
		}
	}
}

// Called every frame
void AUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

float AUnit::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 采矿伤害是给矿脉这类资源用的，单位不吃这一套
	if (DamageEvent.DamageTypeClass && DamageEvent.DamageTypeClass->IsChildOf(UMiningDamageType::StaticClass()))
	{
		return 0.0f;
	}

	// 先让基类按伤害类型算出实际伤害（点伤害/范围伤害的修正、OnTakeAnyDamage 广播都在里面）
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage <= 0.0f || Health <= 0.0f)
	{
		return 0.0f;
	}

	// 护甲减免：打不穿护甲也至少掉 1 点
	const float DamageAfterArmor = (ActualDamage > Armor) ? (ActualDamage - Armor) : 1.0f;
	Health = FMath::Max(0.0f, Health - DamageAfterArmor);

	if (Health <= 0.0f)
	{
		Destroy();   // 阵亡：EndPlay 里会放爆炸表现
	}

	return DamageAfterArmor;
}
