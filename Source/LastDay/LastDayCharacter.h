// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "LastDayCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/** B4-3：玩家死亡广播 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);

/** 玩家手上的工具/武器：数字键 1~4 或滚轮切换 */
UENUM(BlueprintType)
enum class EPlayerTool : uint8
{
	Gun            UMETA(DisplayName = "枪械"),
	MiningTool     UMETA(DisplayName = "采矿器"),
	BuildTurret    UMETA(DisplayName = "建造炮塔"),
	BuildCollector UMETA(DisplayName = "建造采集器")
};

/**
 *  A basic first person character
 */
UCLASS(abstract)
class ALastDayCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
public:
	ALastDayCharacter();

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	// ---------------- B3 射击 ----------------

	/** 摄像机前的方块，充当"枪"的视觉模型（B3-1） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* GunMesh;

	/** 射击左键动作（运行时创建，免去手建 IA/IMC 资产） */
	UPROPERTY() TObjectPtr<UInputMappingContext> RuntimeMappingContext;
	UPROPERTY() TObjectPtr<UInputAction> PrimaryAction;
	UPROPERTY() TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY() TObjectPtr<UInputAction> CancelAction;
	// 工具切换：1=枪械、2=采矿器、3=建造炮塔、4=建造采集器，滚轮上下也可以循环
	UPROPERTY() TObjectPtr<UInputAction> ToolGunAction;
	UPROPERTY() TObjectPtr<UInputAction> ToolMiningAction;
	UPROPERTY() TObjectPtr<UInputAction> ToolBuildTurretAction;
	UPROPERTY() TObjectPtr<UInputAction> ToolBuildCollectorAction;
	UPROPERTY() TObjectPtr<UInputAction> ToolNextAction;
	UPROPERTY() TObjectPtr<UInputAction> ToolPrevAction;

	/** 按下左键：建造模式下放置，否则开始连发 */
	void OnPrimaryAction();

	/** 松开左键：停火 */
	void OnPrimaryActionReleased();

	/** R 键：主动换弹 */
	void OnReloadAction();

	/** 右键：取消建造 */
	void OnCancelAction();

	/** B3-2/B3-3：摄像机射线 + 伤害 + 开火表现 */
	void Shoot();

	/** 开始换弹（正在换弹 / 弹匣已满 / 没有备弹时忽略） */
	void StartReload();

	/** 换弹完成：从备弹里把弹匣补满 */
	void FinishReload();

	/** 临时把弹药数量打到屏幕上（方便没有 HUD 时验证） */
	void ShowAmmoOnScreen();

	/** 换弹时把枪身竖起来，换完再放回去 */
	void UpdateGunReloadPose(float DeltaSeconds);

	// ---------------- 工具 / 武器切换 ----------------

	/** 切换手里的工具（会处理建造模式的进出） */
	UFUNCTION(BlueprintCallable, Category = "Tool")
	void SelectTool(EPlayerTool NewTool);

	/** 滚轮循环切换：Delta = +1 下一个 / -1 上一个 */
	void CycleTool(int32 Delta);

	/** 当前工具 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tool")
	EPlayerTool CurrentTool = EPlayerTool::Gun;

	/** 切换工具的按键包装（Enhanced Input 只接受无参函数） */
	void SelectToolGun() { SelectTool(EPlayerTool::Gun); }
	void SelectToolMining() { SelectTool(EPlayerTool::MiningTool); }
	void SelectToolBuildTurret() { SelectTool(EPlayerTool::BuildTurret); }
	void SelectToolBuildCollector() { SelectTool(EPlayerTool::BuildCollector); }
	void SelectNextTool() { CycleTool(1); }
	void SelectPrevTool() { CycleTool(-1); }

	/** 试着进入某种建造模式；资源不够就退回枪械 */
	void TryEnterBuildMode(TSubclassOf<AActor> InBuildClass, int32 InCost);

	// ---------------- 采矿器 ----------------

	/** 采矿伤害（只有带 UMiningDamageType 的伤害才能从矿脉采集） */
	UPROPERTY(EditAnywhere, Category = "Mining")
	float MiningDamage = 10.0f;

	/** 采矿间隔（秒） */
	UPROPERTY(EditAnywhere, Category = "Mining", meta = (ClampMin = "0.0"))
	float MiningInterval = 0.4f;

	/** 采矿射程（cm） */
	UPROPERTY(EditAnywhere, Category = "Mining", meta = (ClampMin = "0.0"))
	float MiningRange = 1500.0f;

	// ---------------- E1/E2 建造 ----------------

	/** 建造预览体 */
	UPROPERTY() TObjectPtr<AActor> BuildGhost;

	/** 预览体的动态材质，用于绿/红着色 */
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GhostMaterials;

	bool bInBuildMode = false;
	bool bBuildLocationValid = false;
	int32 PendingBuildCost = 0;
	TSubclassOf<AActor> PendingBuildClass;

	void StartBuild(TSubclassOf<AActor> InBuildClass, int32 InCost);
	void CancelBuildInternal();
	void UpdateBuildPreview();
	void UpdateGhostTint(bool bValid);
	void PlacePendingBuilding();

	/** 射速计时（B3-4） */
	float TimeSinceLastShot = 0.0f;

	/** 左键是否按住：按住就按 FireInterval 的节奏连发 */
	bool bFiringHeld = false;

	/** 换弹剩余时间（秒） */
	float ReloadRemaining = 0.0f;

public:

	// ---------------- B4 玩家血量 ----------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 100.0f;

	/** 死亡广播（I1-1 可监听） */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnPlayerDied OnPlayerDied;

	/** 标准受击入口：敌人/子弹的 ApplyDamage 系列接口最终走到这里 */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// ---------------- B3 射击参数 ----------------

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ShootRange = 5000.0f;

	/** 两发之间的间隔（秒）：按住左键就按这个节奏连发 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireInterval = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ShootDamage = 10.0f;

	// ---------------- B3 弹药 ----------------

	/** 一个弹匣多少发 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ammo", meta = (ClampMin = "1"))
	int32 MagazineCapacity = 20;

	/** 初始备弹 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ammo", meta = (ClampMin = "0"))
	int32 ReserveAmmo = 2000;

	/** 换弹时长（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ammo", meta = (ClampMin = "0.0"))
	float ReloadTime = 1.0f;

	/** 当前弹匣内剩余 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Ammo")
	int32 CurrentMagazine = 20;

	/** 当前备弹 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Ammo")
	int32 CurrentReserve = 2000;

	/** 是否正在换弹 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Ammo")
	bool bReloading = false;

	/** 屏幕上是否一直显示 弹匣 / 备弹 */
	UPROPERTY(EditAnywhere, Category = "Combat|Ammo")
	bool bShowAmmoOnScreen = true;

	/** 换弹时枪身竖起来的角度（度，正数枪口朝上） */
	UPROPERTY(EditAnywhere, Category = "Combat|Ammo", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float ReloadGunPitch = 80.0f;

	/** 换弹抬枪 / 放下的转动速度 */
	UPROPERTY(EditAnywhere, Category = "Combat|Ammo", meta = (ClampMin = "0.1"))
	float ReloadGunInterpSpeed = 14.0f;

	// ---------------- B3-3 开火表现 ----------------

	/** 枪口小爆炸的位置：枪模型再往前推这么多（cm） */
	UPROPERTY(EditAnywhere, Category = "Combat|FX")
	float GunMuzzleOffset = 30.0f;

	/** 枪口小爆炸的半径（cm） */
	UPROPERTY(EditAnywhere, Category = "Combat|FX")
	float MuzzleFlashRadius = 6.25f;

	/** 命中处小爆炸的半径（cm） */
	UPROPERTY(EditAnywhere, Category = "Combat|FX")
	float ImpactFlashRadius = 10.0f;

	/** 小爆炸的扩散时长（秒） */
	UPROPERTY(EditAnywhere, Category = "Combat|FX")
	float FlashExpansionTime = 0.02f;

	/** 小爆炸的淡出时长（秒） */
	UPROPERTY(EditAnywhere, Category = "Combat|FX")
	float FlashFadeOutTime = 0.02f;

	// ---------------- E1/E2 建造 ----------------

	/** 按 1 建造的类（默认 ATurret） */
	UPROPERTY(EditAnywhere, Category = "Build")
	TSubclassOf<AActor> TurretBuildClass;

	UPROPERTY(EditAnywhere, Category = "Build")
	int32 TurretCost = 50;

	/** 按 2 建造的类（默认 ACollectorBuilding） */
	UPROPERTY(EditAnywhere, Category = "Build")
	TSubclassOf<AActor> CollectorBuildClass;

	UPROPERTY(EditAnywhere, Category = "Build")
	int32 CollectorCost = 50;

	UPROPERTY(EditAnywhere, Category = "Build")
	float BuildRange = 2000.0f;

	/** 进入炮塔建造模式（E1-1） */
	UFUNCTION(BlueprintCallable, Category = "Build")
	void StartBuildTurret();

	/** 进入采集器建造模式 */
	UFUNCTION(BlueprintCallable, Category = "Build")
	void StartBuildCollector();

	/** 取消建造（E2-2） */
	UFUNCTION(BlueprintCallable, Category = "Build")
	void CancelBuild();

	UFUNCTION(BlueprintPure, Category = "Build")
	bool IsInBuildMode() const { return bInBuildMode; }

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

