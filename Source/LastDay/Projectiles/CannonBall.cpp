// Fill out your copyright notice in the Description page of Project Settings.

#include "CannonBall.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "ExplosionEffect.h"
#include "LastDayVisuals.h"

ACannonBall::ACannonBall()
{
	// 子弹不需要每帧 Tick，交给运动组件处理，节省性能
	PrimaryActorTick.bCanEverTick = false;

	// 初始化默认属性（伤害值由开火的武器覆盖）
	Damage = 10.0f;
	MaxRange = 1000.0f;
	MyShooter = nullptr;

	// 1. 设置碰撞球（根组件）
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	// 用项目自定义的 Projectile 预设：物体类型是 Projectile，
	// 该预设（Config/DefaultEngine.ini）里已经把 Projectile 到 Projectile 的响应设为 Ignore，
	// 所以子弹之间天然互不阻塞，不需要在生成时给每颗子弹维护一份忽略全场子弹的列表。
	// 注意不能沿用 USphereComponent 的默认预设：那套只产生 Overlap，OnHit 永远不会触发。
	CollisionComp->SetCollisionProfileName(TEXT("Projectile"));
	CollisionComp->SetNotifyRigidBodyCollision(true);
	// 绑定碰撞事件
	CollisionComp->OnComponentHit.AddDynamic(this, &ACannonBall::OnHit);
	RootComponent = CollisionComp;

	// 2. 设置模型组件
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionProfileName(TEXT("NoCollision"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> sphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (sphereMesh.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(sphereMesh.Object);
		ProjectileMesh->SetRelativeScale3D(FVector(0.2f, 0.2f, 0.2f));
	}

	// 3. 设置运动组件
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->bRotationFollowsVelocity = true; // 子弹永远头朝向飞行方向
	ProjectileMovement->MaxSpeed = 3000.0f;              // 最大速度
	ProjectileMovement->InitialSpeed = 0.0f;             // 初始速度在生成前覆盖
	ProjectileMovement->ProjectileGravityScale = 1.0f;   // 重力缩放系数

	// 4. 设置生命周期，避免子弹无限存在
	SetLifeSpan(60.0f);
}

void ACannonBall::BeginPlay()
{
	Super::BeginPlay();

	// 统一材质：子弹 = 银色金属
	ApplyLastDayMetalColor(ProjectileMesh, LastDayColors::Metal);

	// 发射者、初始速度、伤害统一在工厂函数 FireProjectile 里处理，这里无需额外逻辑。
}

void ACannonBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// ==========================================
// 命中处理：伤害走官方接口，子弹之间互相忽略
// ==========================================
void ACannonBall::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 打空气、打自己、打发射者：忽略
	if (!OtherActor || OtherActor == this || OtherActor == MyShooter)
	{
		return;
	}

	// 打中的是别的抛射物：直接忽略（不改速度、不伤害、不销毁、不爆炸）。
	// 抛射物之间的碰撞响应已经在 Projectile 预设里设成 Ignore，这里再判一次只是兜底，
	// 不需要提前存一张全场子弹的表。AProjectile 是 ACannonBall / AShooterProjectile 的共同基类。
	if (OtherActor->IsA<AProjectile>())
	{
		return;
	}

	// 官方伤害接口：命中点伤害，带上命中方向、伤害来源与伤害类型
	const FVector ShotDirection = (ProjectileMovement && !ProjectileMovement->Velocity.IsNearlyZero())
		? FVector(ProjectileMovement->Velocity.GetSafeNormal())
		: GetActorForwardVector();

	UGameplayStatics::ApplyPointDamage(
		OtherActor,
		Damage,
		ShotDirection,
		Hit,
		MyShooter ? MyShooter->GetInstigatorController() : nullptr,
		this,
		UDamageType::StaticClass()
	);

	// 命中表现：只放特效。伤害已在上面的 ApplyPointDamage 里结算，
	// 所以这里传 0，避免同一个命中被结算两次。
	if (UWorld* World = GetWorld())
	{
		const FVector ExplosionLocation = Hit.bBlockingHit ? FVector(Hit.ImpactPoint) : GetActorLocation();
		SpawnExplosion(World, AExplosionEffect::StaticClass(), ExplosionLocation, 20.0f, 0.02f, 0.2f, 0.0f);
	}

	// 不管打中什么，最后销毁子弹
	Destroy();
}

ACannonBall* FireProjectile(UWorld* World, TSubclassOf<ACannonBall> ProjectileClass, AActor* Shooter, FVector Location, FRotator Rotation, float Speed, float Damage)
{
	if (!World || !ProjectileClass)
	{
		return nullptr;
	}

	const FTransform SpawnTransform(Rotation, Location);

	// 延迟生成：此时 Actor 已构造，但组件尚未 InitializeComponent()，BeginPlay 也还没跑。
	// 利用这个窗口先写好 InitialSpeed 等参数，组件初始化时就会以它作为真正的起始速度。
	ACannonBall* Projectile = World->SpawnActorDeferred<ACannonBall>(
		ProjectileClass,
		SpawnTransform,
		Shooter,   // Owner：开火者
		nullptr,   // Instigator：AUnit 继承自 AActor 而不是 APawn，这里只能传空
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (!Projectile)
	{
		return nullptr;
	}

	// ---- 子弹初始状态，必须写在 FinishSpawning() 之前 ----
	Projectile->MyShooter = Shooter;   // 记录发射者，命中时用来算伤害归属
	Projectile->Damage = Damage;       // 武器上的伤害值写进子弹
	if (Shooter)
	{
		Projectile->CollisionComp->IgnoreActorWhenMoving(Shooter, true);   // 出膛不炸自己
	}

	// InitializeComponent() 会读到这里设置的 InitialSpeed，它才真正决定初速度
	Projectile->ProjectileMovement->InitialSpeed = Speed;

	// 完成生成：注册组件 → 执行 InitializeComponent()（运动组件在此读取 InitialSpeed）→ BeginPlay
	Projectile->FinishSpawning(SpawnTransform);

	return Projectile;
}
