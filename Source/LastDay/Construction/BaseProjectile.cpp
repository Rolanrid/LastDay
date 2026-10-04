
// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Unit.h"
#include "ExplosionEffect.h"
#include "EngineUtils.h"

ABaseProjectile::ABaseProjectile()
{
	// 子弹不需要每帧 Tick，交给运动组件处理，节省性能
	PrimaryActorTick.bCanEverTick = false;

	// 初始化默认属性
	Damage = 10.0f;
	MaxRange = 1000.0f;

	// 1. 设置碰撞球（透明盒子），作为根组件
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	// 关键：UShapeComponent(Sphere/Box/Capsule) 的默认预设是 OverlapAllDynamic（全 Overlap），
	// 只会产生 Overlap，永远不会产生 blocking hit，OnHit 也就永远不会触发。
	// 必须显式改成阻塞预设，子弹才能真正"撞上"东西。
	CollisionComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComp->SetNotifyRigidBodyCollision(true);
	// 绑定碰撞事件
	CollisionComp->OnComponentHit.AddDynamic(this, &ABaseProjectile::OnHit);
	RootComponent = CollisionComp;

	// 2. 设置模型组件
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionProfileName(TEXT("NoCollision"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> sphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (sphereMesh.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(sphereMesh.Object);
		ProjectileMesh->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));
	}

	// 3. 设置运动组件（UE5 直接生成的运动能力）
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->bRotationFollowsVelocity = true; // 子弹永远头朝向飞行方向
	ProjectileMovement->MaxSpeed = 3000.0f; // 最大速度
	ProjectileMovement->InitialSpeed = 0.0f; // 初始速度（应在FinishSpawning前覆盖）
	ProjectileMovement->ProjectileGravityScale = 1.0f; // 重力缩放系数

	// 4. 设置生命周期，避免子弹无限存在
	SetLifeSpan(60.0f);
}

void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 发射者/初始速度等统一在工厂函数 FireProjectile 里处理，这里无需额外逻辑。
}

void ABaseProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// ==========================================
// 功能 2.2：判断是否打到实体或环境
// ==========================================
void ABaseProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 确保打中的不是空气，也不是子弹自己
	// （工厂函数 FireProjectile 里已经物理忽略了发射者，所以这里不用再判断 != 发射者）
	if ((OtherActor != nullptr) && (OtherActor != this))
	{
		// 尝试判断打中的是不是 AUnit (假设 AUnit 是你的敌人类)
		AUnit* HitUnit = Cast<AUnit>(OtherActor);

		// a) 如果打中了 Unit
		if (HitUnit)
		{
			// 按照朴素的想法，调用 Unit 里面的扣血方法，把 Damage 传过去
			HitUnit->Hitted(Damage);

			// 也可以用 UE5 官方的伤害传递方法（更推荐）：
			// UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, UDamageType::StaticClass());
		}

		// b) 不管打中什么，先在命中点生成一个短暂出现的红色球体作为爆炸表现
		if (UWorld* World = GetWorld())
		{
			const FVector ExplosionLocation = Hit.bBlockingHit ? FVector(Hit.ImpactPoint) : GetActorLocation();
			// 半径 20cm、扩散 0.02s、淡出 0.2s；伤害暂时只记录不结算
			SpawnExplosion(World, AExplosionEffect::StaticClass(), ExplosionLocation, 20.0f, 0.02f, 0.2f, Damage);
		}

		// c) 最后销毁子弹
		Destroy();
	}
}

ABaseProjectile* FireProjectile(UWorld* World, TSubclassOf<ABaseProjectile> ProjectileClass, AUnit* Shooter, FVector Location, FRotator Rotation, float Speed)
{
	if (!World || !Shooter || !ProjectileClass)
	{
		return nullptr;
	}

	const FTransform SpawnTransform(Rotation, Location);

	// 延迟生成：此时 Actor 已构造，但组件尚未 InitializeComponent()，BeginPlay 也还没跑。
	// 利用这个窗口先写好 InitialSpeed 等参数，组件初始化时就会以它作为真正的起始速度。
	ABaseProjectile* Projectile = World->SpawnActorDeferred<ABaseProjectile>(
		ProjectileClass,
		SpawnTransform,
		Shooter,   // Owner：发射它的炮塔
		nullptr,   // Instigator：AUnit 继承自 AActor 而不是 APawn，这里只能传空
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (!Projectile)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Spawn Projectile Failed!"));
		return nullptr;
	}

	// ---- 子弹初始状态，必须写在 FinishSpawning() 之前 ----
	Projectile->MyShooter = Shooter;   // 记录发射者
	Projectile->CollisionComp->IgnoreActorWhenMoving(Shooter, true);   // 出膛不炸

	// 子弹之间互不碰撞：新子弹与场上现存的每一颗子弹互相加入移动忽略列表。
	// 用 MoveIgnoreActors 而不是改碰撞通道——组件扫掠用的通道就是自身的物体类型
	// （见 WorldCollision.cpp 的 ComponentSweepMulti），把子弹通道设成 Ignore 会连墙一起穿。
	for (TActorIterator<ABaseProjectile> It(World); It; ++It)
	{
		ABaseProjectile* ExistingProjectile = *It;
		if (!ExistingProjectile || ExistingProjectile == Projectile)
		{
			continue;
		}

		Projectile->CollisionComp->IgnoreActorWhenMoving(ExistingProjectile, true);
		ExistingProjectile->CollisionComp->IgnoreActorWhenMoving(Projectile, true);
	}

	// InitializeComponent() 会读到这里设置的 InitialSpeed，它才真正决定初速度
	Projectile->ProjectileMovement->InitialSpeed = Speed;

	// 完成生成：注册组件 → 执行 InitializeComponent()（运动组件在此读取 InitialSpeed）→ BeginPlay
	Projectile->FinishSpawning(SpawnTransform);

	return Projectile;
}
