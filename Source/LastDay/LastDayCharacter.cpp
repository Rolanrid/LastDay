// Copyright Epic Games, Inc. All Rights Reserved.

#include "LastDayCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DemoBuildings.h"
#include "ExplosionEffect.h"
#include "LastDay.h"
#include "LastDayVisuals.h"
#include "MiningDamageType.h"
#include "LastDayGameState.h"
#include "Turret.h"

ALastDayCharacter::ALastDayCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	// B2-5：移动/跳跃参数，只让 Yaw 跟随控制器（Pitch/Roll 由摄像机处理）
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->JumpZVelocity = 420.0f;
	GetCharacterMovement()->AirControl = 0.5f;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// B3-1：摄像机前方挂一个方块当"枪"（纯装饰）
	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	GunMesh->SetupAttachment(FirstPersonCameraComponent);
	GunMesh->SetCollisionProfileName(FName("NoCollision"));
	GunMesh->SetRelativeLocation(FVector(24.0f, 12.0f, -10.0f));
	GunMesh->SetRelativeScale3D(FVector(0.35f, 0.08f, 0.08f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GunCubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (GunCubeMesh.Succeeded())
	{
		GunMesh->SetStaticMesh(GunCubeMesh.Object);
	}

	// E1/E2：默认的可建造物
	TurretBuildClass = ATurret::StaticClass();
	CollectorBuildClass = ACollectorBuilding::StaticClass();

	// B4-1：初始血量满
	CurrentHealth = MaxHealth;
}

void ALastDayCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 弹药初始化：弹匣补满、备弹按配置来（这样蓝图里改 MagazineCapacity / ReserveAmmo 也生效）
	CurrentMagazine = MagazineCapacity;
	CurrentReserve = ReserveAmmo;
	TimeSinceLastShot = FireInterval;   // 开局就能立刻开火
	bFiringHeld = false;
	bReloading = false;
	ReloadRemaining = 0.0f;

	// 统一材质：武器（手里的枪）= 银色金属
	ApplyLastDayMetalColor(GunMesh, LastDayColors::Metal);

	// 按当前工具把手上状态摆好（枪的显隐、建造预览）
	SelectTool(CurrentTool);
}

void ALastDayCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ALastDayCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ALastDayCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALastDayCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALastDayCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ALastDayCharacter::LookInput);

		// ---- 运行时创建的输入（免去手建 IA/IMC 资产）----
		// 左键=开火(按住连发)/放置，R=换弹，右键=取消建造
		// 1=枪械，2=采矿器，3=建造炮塔，4=建造采集器，滚轮上下循环切换
		RuntimeMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_DemoRuntime"));

		auto MakeBoolAction = [this](const TCHAR* ActionName, const FKey& Key) -> UInputAction*
		{
			UInputAction* Action = NewObject<UInputAction>(this, ActionName);
			Action->ValueType = EInputActionValueType::Boolean;
			RuntimeMappingContext->MapKey(Action, Key);
			return Action;
		};

		PrimaryAction = MakeBoolAction(TEXT("IA_DemoPrimary"), EKeys::LeftMouseButton);
		ReloadAction = MakeBoolAction(TEXT("IA_DemoReload"), EKeys::R);
		CancelAction = MakeBoolAction(TEXT("IA_DemoCancel"), EKeys::RightMouseButton);
		ToolGunAction = MakeBoolAction(TEXT("IA_DemoToolGun"), EKeys::One);
		ToolMiningAction = MakeBoolAction(TEXT("IA_DemoToolMining"), EKeys::Two);
		ToolBuildTurretAction = MakeBoolAction(TEXT("IA_DemoToolBuildTurret"), EKeys::Three);
		ToolBuildCollectorAction = MakeBoolAction(TEXT("IA_DemoToolBuildCollector"), EKeys::Four);
		ToolNextAction = MakeBoolAction(TEXT("IA_DemoToolNext"), EKeys::MouseScrollUp);
		ToolPrevAction = MakeBoolAction(TEXT("IA_DemoToolPrev"), EKeys::MouseScrollDown);

		EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Started, this, &ALastDayCharacter::OnPrimaryAction);
		EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &ALastDayCharacter::OnPrimaryActionReleased);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ALastDayCharacter::OnReloadAction);
		EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Started, this, &ALastDayCharacter::OnCancelAction);
		EnhancedInputComponent->BindAction(ToolGunAction, ETriggerEvent::Started, this, &ALastDayCharacter::SelectToolGun);
		EnhancedInputComponent->BindAction(ToolMiningAction, ETriggerEvent::Started, this, &ALastDayCharacter::SelectToolMining);
		EnhancedInputComponent->BindAction(ToolBuildTurretAction, ETriggerEvent::Started, this, &ALastDayCharacter::SelectToolBuildTurret);
		EnhancedInputComponent->BindAction(ToolBuildCollectorAction, ETriggerEvent::Started, this, &ALastDayCharacter::SelectToolBuildCollector);
		EnhancedInputComponent->BindAction(ToolNextAction, ETriggerEvent::Started, this, &ALastDayCharacter::SelectNextTool);
		EnhancedInputComponent->BindAction(ToolPrevAction, ETriggerEvent::Started, this, &ALastDayCharacter::SelectPrevTool);

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
				{
					Subsystem->AddMappingContext(RuntimeMappingContext, 10);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogLastDay, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ALastDayCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void ALastDayCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void ALastDayCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ALastDayCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void ALastDayCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void ALastDayCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

// =====================================================
// B4：玩家血量
// =====================================================

float ALastDayCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (CurrentHealth <= 0.0f)
	{
		return 0.0f;
	}

	// 让基类先按伤害类型算出实际伤害
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - ActualDamage);

	if (CurrentHealth <= 0.0f)
	{
		OnPlayerDied.Broadcast();

		// I1-1：直接结束流程（失败）
		if (ALastDayGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ALastDayGameState>() : nullptr)
		{
			GameState->TriggerGameOver(false);
		}

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DisableInput(PC);
		}
		GetCharacterMovement()->DisableMovement();
	}

	return ActualDamage;
}

// =====================================================
// B3：射击
// =====================================================

void ALastDayCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeSinceLastShot += DeltaSeconds;

	// 换弹计时
	if (bReloading)
	{
		ReloadRemaining -= DeltaSeconds;
		if (ReloadRemaining <= 0.0f)
		{
			FinishReload();
		}
	}

	// 按住左键就按 FireInterval 的节奏连发（间隔与弹药都由 Shoot 自己判断）
	if (bFiringHeld && !bInBuildMode)
	{
		Shoot();
	}

	if (bInBuildMode)
	{
		UpdateBuildPreview();
	}

	// 弹药数量一直挂在屏幕上，换弹时枪身竖起来
	ShowAmmoOnScreen();
	UpdateGunReloadPose(DeltaSeconds);
}

void ALastDayCharacter::OnPrimaryAction()
{
	// 建造类工具：左键放东西
	if (CurrentTool == EPlayerTool::BuildTurret || CurrentTool == EPlayerTool::BuildCollector)
	{
		PlacePendingBuilding();   // E2-1
		return;
	}

	// 枪械 / 采矿器：按住连发，按下先来一下，之后由 Tick 按间隔继续
	bFiringHeld = true;
	Shoot();
}

void ALastDayCharacter::OnPrimaryActionReleased()
{
	bFiringHeld = false;
}

void ALastDayCharacter::OnReloadAction()
{
	StartReload();
}

void ALastDayCharacter::Shoot()
{
	const bool bMining = (CurrentTool == EPlayerTool::MiningTool);

	// B3-4：射速限制（枪用 FireInterval，采矿器用 MiningInterval）
	const float Interval = bMining ? MiningInterval : FireInterval;
	if (TimeSinceLastShot < Interval)
	{
		return;
	}

	// 换弹和弹药只和枪械有关
	if (!bMining)
	{
		if (bReloading)
		{
			return;
		}

		// 弹匣打空就自动换弹
		if (CurrentMagazine <= 0)
		{
			StartReload();
			return;
		}

		--CurrentMagazine;
	}

	TimeSinceLastShot = 0.0f;

	const FVector Forward = FirstPersonCameraComponent->GetForwardVector();
	const FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	const FVector End = Start + Forward * (bMining ? MiningRange : ShootRange);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerShoot), false, this);
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	UWorld* World = GetWorld();

	// B3-3：开火表现——枪口一个小爆炸（不管打没打中都有）
	if (World && GunMesh)
	{
		const FVector MuzzleLocation = GunMesh->GetComponentLocation() + Forward * GunMuzzleOffset;
		SpawnExplosion(World, AExplosionEffect::StaticClass(), MuzzleLocation, MuzzleFlashRadius, FlashExpansionTime, FlashFadeOutTime, 0.0f);
	}

	if (bHit)
	{
		// 官方伤害接口：命中点伤害
		// 枪械用普通伤害（打单位），采矿器用 UMiningDamageType（只有矿脉吃这个伤害）
		if (AActor* HitActor = Hit.GetActor())
		{
			UGameplayStatics::ApplyPointDamage(
				HitActor,
				bMining ? MiningDamage : ShootDamage,
				Forward,
				Hit,
				GetController(),
				this,
				bMining ? UMiningDamageType::StaticClass() : UDamageType::StaticClass()
			);
		}

		// 命中处再放一个小爆炸：只做表现，伤害已经在上面结算过
		if (World)
		{
			const FVector ImpactLocation = FVector(Hit.ImpactPoint) + Hit.ImpactNormal * (ImpactFlashRadius * 0.25f);
			SpawnExplosion(World, AExplosionEffect::StaticClass(), ImpactLocation, ImpactFlashRadius, FlashExpansionTime, FlashFadeOutTime, 0.0f);
		}
	}
}

// =====================================================
// 工具 / 武器切换
// =====================================================

void ALastDayCharacter::SelectTool(EPlayerTool NewTool)
{
	CurrentTool = NewTool;
	bFiringHeld = false;

	switch (NewTool)
	{
	case EPlayerTool::BuildTurret:
		TryEnterBuildMode(TurretBuildClass, TurretCost);
		break;

	case EPlayerTool::BuildCollector:
		TryEnterBuildMode(CollectorBuildClass, CollectorCost);
		break;

	default:
		// 非建造工具：收拾掉建造预览
		if (bInBuildMode)
		{
			CancelBuildInternal();
		}
		break;
	}

	// 枪只在"枪械 / 采矿器"下露出来，建造时手里是预览体
	if (GunMesh)
	{
		GunMesh->SetVisibility(CurrentTool == EPlayerTool::Gun || CurrentTool == EPlayerTool::MiningTool);
	}
}

void ALastDayCharacter::CycleTool(int32 Delta)
{
	const int32 ToolCount = 4;
	const int32 CurrentIndex = static_cast<int32>(CurrentTool);
	const int32 NextIndex = ((CurrentIndex + Delta) % ToolCount + ToolCount) % ToolCount;
	SelectTool(static_cast<EPlayerTool>(NextIndex));
}

void ALastDayCharacter::TryEnterBuildMode(TSubclassOf<AActor> InBuildClass, int32 InCost)
{
	// 已经在建同一种东西就不用重来
	if (bInBuildMode && PendingBuildClass == InBuildClass)
	{
		return;
	}

	StartBuild(InBuildClass, InCost);

	// 没进成（比如源晶不够）就退回枪械
	if (!bInBuildMode)
	{
		CurrentTool = EPlayerTool::Gun;
		if (GunMesh)
		{
			GunMesh->SetVisibility(true);
		}
	}
}

// =====================================================
// B3：弹药与换弹
// =====================================================

void ALastDayCharacter::StartReload()
{
	if (bReloading || CurrentMagazine >= MagazineCapacity || CurrentReserve <= 0)
	{
		return;
	}

	bReloading = true;
	ReloadRemaining = FMath::Max(ReloadTime, 0.0f);

	// 换弹时间配置成 0 就立刻完成
	if (ReloadRemaining <= 0.0f)
	{
		FinishReload();
	}
}

void ALastDayCharacter::FinishReload()
{
	const int32 Needed = FMath::Max(0, MagazineCapacity - CurrentMagazine);
	const int32 Loaded = FMath::Min(Needed, CurrentReserve);

	CurrentMagazine += Loaded;
	CurrentReserve -= Loaded;
	bReloading = false;
	ReloadRemaining = 0.0f;
}

void ALastDayCharacter::ShowAmmoOnScreen()
{
	// 屏幕上一直显示当前工具 / 弹药（没有正式 UI 时的占位），以后接了 UI 可以关掉
	if (!bShowAmmoOnScreen || !GEngine)
	{
		return;
	}

	FString ToolName;
	FString Detail;
	switch (CurrentTool)
	{
	case EPlayerTool::MiningTool:
		ToolName = TEXT("采矿器");
		Detail = FString::Printf(TEXT("射程 %.0f  间隔 %.2fs（对准矿脉采集）"), MiningRange, MiningInterval);
		break;

	case EPlayerTool::BuildTurret:
		ToolName = TEXT("建造·炮塔");
		Detail = FString::Printf(TEXT("左键放置（花费 %d 源晶），右键取消"), TurretCost);
		break;

	case EPlayerTool::BuildCollector:
		ToolName = TEXT("建造·采集器");
		Detail = FString::Printf(TEXT("左键放置（花费 %d 源晶），右键取消"), CollectorCost);
		break;

	case EPlayerTool::Gun:
	default:
		ToolName = TEXT("枪械");
		Detail = bReloading
			? FString::Printf(TEXT("弹匣 %d / %d    备弹 %d    [换弹中…]"), CurrentMagazine, MagazineCapacity, CurrentReserve)
			: FString::Printf(TEXT("弹匣 %d / %d    备弹 %d"), CurrentMagazine, MagazineCapacity, CurrentReserve);
		break;
	}

	// key 1002：和 GameState 的状态文字（1001）分开，不会互相覆盖
	GEngine->AddOnScreenDebugMessage(1002, 0.0f, bReloading ? FColor::Orange : FColor::Yellow,
		FString::Printf(TEXT("【%s】%s"), *ToolName, *Detail));
}

void ALastDayCharacter::UpdateGunReloadPose(float DeltaSeconds)
{
	if (!GunMesh)
	{
		return;
	}

	// 换弹时枪身竖起来（枪口朝上），换完再转回去
	const FRotator Target = bReloading ? FRotator(ReloadGunPitch, 0.0f, 0.0f) : FRotator::ZeroRotator;
	const FRotator Current = GunMesh->GetRelativeRotation();

	if (!Current.Equals(Target, 0.05f))
	{
		GunMesh->SetRelativeRotation(FMath::RInterpTo(Current, Target, DeltaSeconds, ReloadGunInterpSpeed));
	}
}

// =====================================================
// E1/E2：建造
// =====================================================

void ALastDayCharacter::StartBuildTurret()
{
	SelectTool(EPlayerTool::BuildTurret);
}

void ALastDayCharacter::StartBuildCollector()
{
	SelectTool(EPlayerTool::BuildCollector);
}

void ALastDayCharacter::StartBuild(TSubclassOf<AActor> InBuildClass, int32 InCost)
{
	bFiringHeld = false;   // 进入建造模式就停火

	if (bInBuildMode)
	{
		CancelBuildInternal();   // 只清掉上一个预览体，不要动当前工具
	}

	UWorld* World = GetWorld();
	if (!World || !InBuildClass)
	{
		return;
	}

	// 资源不够就不进建造模式
	if (ALastDayGameState* GameState = World->GetGameState<ALastDayGameState>())
	{
		if (GameState->PlayerCrystal < InCost)
		{
			UE_LOG(LogLastDay, Warning, TEXT("[Build] 源晶不足（需要 %d，当前 %d）"), InCost, GameState->PlayerCrystal);
			return;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	BuildGhost = World->SpawnActor<AActor>(InBuildClass, FTransform(FRotator::ZeroRotator, GetActorLocation() + FVector(0.0f, 0.0f, 300.0f)), SpawnParams);
	if (!BuildGhost)
	{
		return;
	}

	// 预览体只用于显示：不参与碰撞、不 Tick（炮台预览不会开火）
	BuildGhost->SetActorEnableCollision(false);
	BuildGhost->SetActorTickEnabled(false);

	// 缓存动态材质，后面只改颜色
	// （炮口是挂在预览体上的独立 Actor，也要一起收集）
	GhostMaterials.Reset();
	TArray<AActor*> GhostActors;
	GhostActors.Add(BuildGhost);
	BuildGhost->GetAttachedActors(GhostActors);

	for (AActor* GhostActor : GhostActors)
	{
		TInlineComponentArray<UPrimitiveComponent*> Primitives;
		GhostActor->GetComponents(Primitives);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (UMaterialInstanceDynamic* DynMat = Primitive->CreateAndSetMaterialInstanceDynamic(0))
			{
				GhostMaterials.Add(DynMat);
			}
		}
	}

	PendingBuildClass = InBuildClass;
	PendingBuildCost = InCost;
	bInBuildMode = true;
	bBuildLocationValid = false;
	UpdateGhostTint(false);
}

void ALastDayCharacter::CancelBuild()
{
	CancelBuildInternal();
	SelectTool(EPlayerTool::Gun);   // 取消建造后回到枪械
}

void ALastDayCharacter::CancelBuildInternal()
{
	if (BuildGhost)
	{
		BuildGhost->Destroy();
		BuildGhost = nullptr;
	}

	GhostMaterials.Reset();
	PendingBuildClass = nullptr;
	PendingBuildCost = 0;
	bInBuildMode = false;
	bBuildLocationValid = false;
}

void ALastDayCharacter::OnCancelAction()
{
	if (bInBuildMode)
	{
		CancelBuild();   // E2-2：取消并回到枪械
	}
}

void ALastDayCharacter::UpdateBuildPreview()
{
	if (!BuildGhost)
	{
		return;
	}

	// E1-2：从摄像机打一条射线，把预览体放到地面命中点
	const FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	const FVector End = Start + FirstPersonCameraComponent->GetForwardVector() * BuildRange;

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(BuildTrace), false, this);
	FHitResult Hit;
	bBuildLocationValid = false;

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, TraceParams) && Hit.bBlockingHit)
	{
		// 抬高半个身位，让方块底面正好贴地
		const FVector PlaceLocation = FVector(Hit.ImpactPoint) + FVector(0.0f, 0.0f, 55.0f);
		BuildGhost->SetActorLocation(PlaceLocation);

		// 重叠检测：能放下才合法
		FCollisionQueryParams OverlapParams(SCENE_QUERY_STAT(BuildCheck), false, this);
		OverlapParams.AddIgnoredActor(BuildGhost);
		const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
			PlaceLocation, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeBox(FVector(45.0f)), OverlapParams);

		bBuildLocationValid = !bBlocked;
	}

	UpdateGhostTint(bBuildLocationValid);
}

void ALastDayCharacter::UpdateGhostTint(bool bValid)
{
	const FLinearColor TintColor = bValid ? FLinearColor(0.05f, 1.0f, 0.1f, 1.0f) : FLinearColor(1.0f, 0.05f, 0.05f, 1.0f);

	for (UMaterialInstanceDynamic* DynMat : GhostMaterials)
	{
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(TEXT("Color"), TintColor);
		}
	}
}

void ALastDayCharacter::PlacePendingBuilding()
{
	UWorld* World = GetWorld();
	if (!bInBuildMode || !BuildGhost || !World)
	{
		return;
	}

	if (!bBuildLocationValid)
	{
		UE_LOG(LogLastDay, Warning, TEXT("[Build] 当前位置不能放置"));
		return;
	}

	// E2-1：扣资源 → 生成真实建筑
	if (ALastDayGameState* GameState = World->GetGameState<ALastDayGameState>())
	{
		if (!GameState->ConsumeCrystal(PendingBuildCost))
		{
			UE_LOG(LogLastDay, Warning, TEXT("[Build] 源晶不足，无法放置"));
			return;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<AActor>(PendingBuildClass, BuildGhost->GetActorTransform(), SpawnParams);

	CancelBuildInternal();
}
