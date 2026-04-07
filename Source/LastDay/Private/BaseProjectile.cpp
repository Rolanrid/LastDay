
// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Unit.h" 

ABaseProjectile::ABaseProjectile()
{
	// 子弹不需要每帧 Tick，交给运动组件处理，节省性能
	PrimaryActorTick.bCanEverTick = false;

	// 初始化默认属性
	Damage = 10.0f;
	MaxRange = 1000.0f;

	// 1. 设置碰撞球（透明盒子），作为根组件
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(10.0f); // 碰撞盒子大小
	// 绑定碰撞事件
	CollisionComp->OnComponentHit.AddDynamic(this, &ABaseProjectile::OnHit);
	RootComponent = CollisionComp;

	// 2. 设置模型组件
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("/Engine/BasicShapes/Cylinder")); // 暂用球替代
	ProjectileMesh->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));
	ProjectileMesh->SetupAttachment(RootComponent);
	// 模型不需要物理碰撞，碰撞全靠外面的球
	ProjectileMesh->SetCollisionProfileName(TEXT("NoCollision"));

	// 3. 设置运动组件（UE5 直接生成的运动能力）
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->bRotationFollowsVelocity = true; // 子弹永远头朝向飞行方向
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 重力缩放系数
	// 注意：初始速度我们先不设，等炮塔调用 FireProjectile 时再赋给它！
}

void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 之前这里用的 GetOwner 已经去掉了，我们把它移到了 FireProjectile 接口里面统一处理！
}

void ABaseProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// ==========================================
// 功能 1：炮塔调用子弹的接口
// ==========================================
void ABaseProjectile::FireProjectile(AUnit* Shooter, float ShootSpeed)
{
	if (ProjectileMovement)
	{
		// 记录发射者是谁，并在物理世界中完全忽略它，防止出膛就炸（解答你的第1点要求）
		MyShooter = Shooter;
		if (MyShooter && CollisionComp)
		{
			CollisionComp->IgnoreActorWhenMoving(MyShooter, true);
		}

		// 赋予速度和方向，它自己就会飞啦！
		ProjectileMovement->InitialSpeed = ShootSpeed;
		ProjectileMovement->ProjectileGravityScale = 1.0f; // 重力缩放系数

		SetLifeSpan(5.0f);
	}
}

// ==========================================
// 功能 2.2：判断是否打到实体或环境
// ==========================================
void ABaseProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 确保打中的不是空气，也不是子弹自己
	// （因为我们在 FireProjectile 里物理忽略了 MyShooter，所以这里连判断 != 发射者都不用写啦！解答你的第4点要求）
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

		// b) 如果没打中 Unit（也就是 HitUnit 为空），那就说明打到了环境（墙壁、地面等透明盒子）
		// 不管打中什么，最后都直接销毁子弹
		Destroy();
	}
}

ABaseProjectile* SpawnProjectile(UWorld* World, AUnit* Shooter, FVector Location, FRotator Rotation, float Speed)
{
	if (!World || !Shooter)
	{
		return nullptr;
	}

	// 生成 Actor
	ABaseProjectile* Projectile = World->SpawnActor<ABaseProjectile>(Location, Rotation);
	if (!Projectile)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Spawn Projectile!"));
		return nullptr;
	}

	Projectile->SetActorLocation(Location);
	Projectile->SetActorRotation(Rotation); // 旋转使圆柱体指向 X 轴
	// 调用初始化函数设置参数
	Projectile->FireProjectile(Shooter, Speed);
	return Projectile;
}