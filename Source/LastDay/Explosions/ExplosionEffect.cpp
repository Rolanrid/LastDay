// Fill out your copyright notice in the Description page of Project Settings.

#include "ExplosionEffect.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "LastDayEditorAssets.h"

namespace
{
	// 引擎白模球（/Engine/BasicShapes/Sphere）的半径是 50cm
	constexpr float MeshRadius = 50.0f;

	// 爆炸颜色：橙红（#FF4500 在线性空间的等价色，直接写线性值容易偏色）
	const FLinearColor ExplosionColor = FLinearColor::FromSRGBColor(FColor(255, 69, 0));
}

AExplosionEffect::AExplosionEffect()
{
	PrimaryActorTick.bCanEverTick = true;

	Radius = 60.0f;
	ExpansionTime = 0.2f; // 扩散时长：从 0 膨胀到 Radius 用多久
	FadeOutTime = 0.6f;   // 淡出时长
	Damage = 0.0f;        // 暂时只记录，不参与结算
	ExplosionMaterial = nullptr;
	CurrentRadius = 0.0f;
	ExpansionElapsed = 0.0f;
	FadeElapsed = 0.0f;
	DynamicMaterial = nullptr;

	// 根组件：引擎自带的球体白模
	SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExplosionSphere"));
	RootComponent = SphereMesh;
	SphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereMesh->SetCollisionProfileName(TEXT("NoCollision"));
	SphereMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> sphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere"));
	if (sphereMeshAsset.Succeeded())
	{
		SphereMesh->SetStaticMesh(sphereMeshAsset.Object);
	}

	// 这里用代码自动生成的 /Game/FX/M_Explosion（Translucent + Unlit + Color/Opacity 参数，
	// 由 LastDayEditorAssets 在编辑器启动时创建）；拿不到就先用基础白模材质占位。
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> explosionMaterial(TEXT("/Game/FX/M_Explosion"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> basicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));

	if (ExplosionMaterial)
	{
		SphereMesh->SetMaterial(0, ExplosionMaterial);
	}
	else if (explosionMaterial.Succeeded())
	{
		ExplosionMaterial = explosionMaterial.Object;
		SphereMesh->SetMaterial(0, explosionMaterial.Object);
	}
	else if (basicShapeMaterial.Succeeded())
	{
		SphereMesh->SetMaterial(0, basicShapeMaterial.Object);
	}
}

void AExplosionEffect::BeginPlay()
{
	Super::BeginPlay();

	// 编辑器下首次运行时补齐 /Game/FX/M_Explosion（重启编辑器时模块启动也会生成，
	// 这里兜底是为了热编译后不用重启就能生效）；非编辑器构建里这是个空函数。
	EnsureLastDayEditorAssets();

	// 兜底：CDO 构造时 /Game/FX/M_Explosion 可能还不存在（首次运行时才生成），这里再找一次
	if (!ExplosionMaterial)
	{
		ExplosionMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FX/M_Explosion.M_Explosion"));
	}
	if (ExplosionMaterial)
	{
		SphereMesh->SetMaterial(0, ExplosionMaterial);
	}

	// 用动态材质把球体染成橙红，并拿到句柄以便后面改透明度
	DynamicMaterial = SphereMesh->CreateAndSetMaterialInstanceDynamic(0);
	if (DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), ExplosionColor);
		DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
	}

	// 白模球半径是 50，缩放系数 = 半径 / 50
	CurrentRadius = FMath::Min(1.0f, Radius);   // 起点给个极小值，避免出现 0 缩放
	ExpansionElapsed = 0.0f;
	FadeElapsed = 0.0f;
	SphereMesh->SetWorldScale3D(FVector(CurrentRadius / MeshRadius));

	// 伤害结算：Damage > 0 时按官方接口做一次范围伤害
	// （子弹命中的表现特效传 0，所以不会和子弹本身的命中伤害重复结算）
	if (Damage > 0.0f)
	{
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		UGameplayStatics::ApplyRadialDamage(
			this,
			Damage,
			GetActorLocation(),
			Radius,
			UDamageType::StaticClass(),
			IgnoredActors,
			this,        // DamageCauser：爆炸表现本身
			nullptr,     // InstigatorController：AUnit 不是 APawn，没有控制器
			true         // bDoFullDamage：作用范围内不做衰减
		);
	}

	// 兜底：参数非法时用一个固定存活时间，避免爆炸球一直残留
	if (Radius <= 0.0f)
	{
		SetLifeSpan(FadeOutTime);
	}
}

void AExplosionEffect::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 参数非法时交给 BeginPlay 里设置的兜底寿命
	if (Radius <= 0.0f)
	{
		return;
	}

	// 阶段一：在 ExpansionTime 内从 0 均匀扩散到 Radius
	if (CurrentRadius < Radius)
	{
		ExpansionElapsed += DeltaTime;
		const float ExpansionAlpha = FMath::Clamp(ExpansionElapsed / FMath::Max(ExpansionTime, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
		CurrentRadius = Radius * ExpansionAlpha;
		SphereMesh->SetWorldScale3D(FVector(CurrentRadius / MeshRadius));
		return;
	}

	// 阶段二：在 FadeOutTime 内逐渐变透明，完全看不见再销毁
	FadeElapsed += DeltaTime;
	const float FadeAlpha = 1.0f - FMath::Clamp(FadeElapsed / FMath::Max(FadeOutTime, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), FadeAlpha);
	}

	if (FadeAlpha <= 0.0f)
	{
		Destroy();
	}
}

AExplosionEffect* SpawnExplosion(UWorld* World, TSubclassOf<AExplosionEffect> ExplosionClass, FVector Location, float Radius, float ExpansionTime, float FadeOutTime, float Damage)
{
	if (!World || !ExplosionClass)
	{
		return nullptr;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, Location);

	// 延迟生成：组件尚未 InitializeComponent，先把参数写好（与子弹工厂同一套模式）
	AExplosionEffect* Explosion = World->SpawnActorDeferred<AExplosionEffect>(
		ExplosionClass,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (!Explosion)
	{
		return nullptr;
	}

	// 必须在 FinishSpawning() 之前，BeginPlay / Tick 才会用到这些值
	Explosion->Radius = Radius;
	Explosion->ExpansionTime = ExpansionTime;
	Explosion->FadeOutTime = FadeOutTime;
	Explosion->Damage = Damage;   // > 0 时 BeginPlay 会结算一次范围伤害

	Explosion->FinishSpawning(SpawnTransform);

	return Explosion;
}
