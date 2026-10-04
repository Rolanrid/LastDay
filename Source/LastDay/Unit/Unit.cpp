// Fill out your copyright notice in the Description page of Project Settings.


#include "Unit.h"
#include "Components/BoxComponent.h"
#include "ExplosionEffect.h"

// Sets default values
AUnit::AUnit()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Hp = 100;
	Armor = 5;

	// 受击体：尺寸与项目里常用的白模方块（引擎 Cube，100³）一致，保证任何单位都能被命中
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("UnitCollision"));
	CollisionComponent->InitBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	// 必须是"阻塞"预设，子弹的 OnHit 才会触发；纯 Overlap 预设只会触发 OnComponentBeginOverlap
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
	
}

void AUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 单位被销毁（阵亡）时放一个爆炸表现；关卡切换/退出编辑器时不放
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		if (UWorld* World = GetWorld())
		{
			// 半径 200cm、扩散 1.0s、淡出 1.0s；伤害暂时只记录不结算
			SpawnExplosion(World, AExplosionEffect::StaticClass(), GetActorLocation(), 150.0f, 1.0f, 1.0f, 0.0f);
		}
	}
}

// Called every frame
void AUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AUnit::Hitted(uint64_t damage)
{
	if (damage > Armor) {
		damage -= Armor;
	} else {
		damage = 1;
	}
	if (damage > Hp) {
		Hp = 0;
		Destroy();
	} else {
		Hp -= damage;
	}
}
