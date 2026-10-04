#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Character.h"
#include "WPSoldierCharacter.generated.h"

class AWPBattlefieldDirector;
class AWPFragGrenade;
class AWPSmokeGrenade;
class UCameraComponent;
class UStaticMeshComponent;
class UWPHealthComponent;
class UWPSuppressionComponent;
class UWPWeaponComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPSoldierSimpleEventSignature);

UCLASS()
class WARRIORPARADISE_API AWPSoldierCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AWPSoldierCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintPure, Category="Combat")
    UWPHealthComponent* GetHealthComponent() const { return HealthComponent; }

    UFUNCTION(BlueprintPure, Category="Combat")
    UWPWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

    UFUNCTION(BlueprintPure, Category="Combat")
    UWPSuppressionComponent* GetSuppressionComponent() const { return SuppressionComponent; }

    UFUNCTION(BlueprintPure, Category="Combat")
    EWPCombatTeam GetCombatTeam() const { return CombatTeam; }

    UFUNCTION(BlueprintCallable, Category="Combat")
    void SetCombatTeam(EWPCombatTeam NewTeam) { CombatTeam = NewTeam; }

    UFUNCTION(BlueprintPure, Category="Combat")
    EWPSoldierRole GetSoldierRole() const { return SoldierRole; }

    UFUNCTION(BlueprintCallable, Category="Combat")
    void SetSoldierRole(EWPSoldierRole NewRole) { SoldierRole = NewRole; }

    UFUNCTION(BlueprintPure, Category="Combat")
    bool IsDowned() const;

    UFUNCTION(BlueprintPure, Category="Combat")
    bool IsCombatCapable() const;

    UFUNCTION(BlueprintPure, Category="Combat")
    int32 GetGrenadesRemaining() const { return GrenadesRemaining; }

    UFUNCTION(BlueprintPure, Category="Combat")
    int32 GetSmokeGrenadesRemaining() const { return SmokeGrenadesRemaining; }

    UFUNCTION(BlueprintPure, Category="Combat")
    int32 GetMedkitsRemaining() const { return MedkitsRemaining; }

    UFUNCTION(BlueprintPure, Category="Combat")
    bool IsBandaging() const { return bBandaging; }

    UPROPERTY(BlueprintAssignable, Category="Combat|Presentation")
    FWPSoldierSimpleEventSignature OnMeleeAttack;

    UPROPERTY(BlueprintAssignable, Category="Combat|Presentation")
    FWPSoldierSimpleEventSignature OnBandageStarted;

    UPROPERTY(BlueprintAssignable, Category="Combat|Presentation")
    FWPSoldierSimpleEventSignature OnBandageCompleted;

    UFUNCTION(BlueprintPure, Category="Movement")
    float GetStaminaNormalized() const
    {
        return MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f;
    }

    UFUNCTION(BlueprintPure, Category="Movement")
    bool IsSprinting() const { return bSprinting; }

    UFUNCTION(BlueprintPure, Category="Movement")
    bool IsProne() const { return bProne; }

    UFUNCTION(BlueprintPure, Category="Movement")
    bool IsCrouchedStance() const { return bIsCrouched; }

    UFUNCTION(BlueprintCallable, Category="Combat")
    bool ThrowGrenadeAt(const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category="Combat")
    bool ThrowSmokeGrenadeAt(const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category="Combat")
    bool TryReviveSoldier(AWPSoldierCharacter* Target);

    UFUNCTION(BlueprintCallable, Category="Combat")
    bool TryMeleeAttack(AWPSoldierCharacter* Target);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="View")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
    TObjectPtr<UWPHealthComponent> HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
    TObjectPtr<UWPWeaponComponent> WeaponComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
    TObjectPtr<UWPSuppressionComponent> SuppressionComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    EWPCombatTeam CombatTeam = EWPCombatTeam::Allies;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    EWPSoldierRole SoldierRole = EWPSoldierRole::Rifleman;

private:
    UPROPERTY(VisibleAnywhere, Category="View")
    TObjectPtr<UStaticMeshComponent> ViewWeaponBody;

    UPROPERTY(VisibleAnywhere, Category="View")
    TObjectPtr<UStaticMeshComponent> ViewWeaponBarrel;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Grenade")
    TSubclassOf<AWPFragGrenade> GrenadeClass;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Grenade")
    TSubclassOf<AWPSmokeGrenade> SmokeGrenadeClass;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Grenade", meta=(ClampMin="0"))
    int32 GrenadesRemaining = 2;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Grenade", meta=(ClampMin="0"))
    int32 SmokeGrenadesRemaining = 1;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Grenade", meta=(ClampMin="100.0"))
    float GrenadeThrowSpeed = 1350.0f;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Interaction", meta=(ClampMin="50.0"))
    float ReviveInteractionRange = 225.0f;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Melee", meta=(ClampMin="1.0"))
    float MeleeDamage = 65.0f;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Melee", meta=(ClampMin="50.0"))
    float MeleeRange = 185.0f;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Melee", meta=(ClampMin="0.1"))
    float MeleeCooldownSeconds = 0.85f;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Medicine", meta=(ClampMin="0"))
    int32 MedkitsRemaining = 1;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Medicine", meta=(ClampMin="0.1"))
    float BandageDurationSeconds = 2.6f;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Medicine", meta=(ClampMin="1.0"))
    float BandageHealAmount = 52.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float WalkSpeed = 470.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float SprintSpeed = 690.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float ProneSpeed = 145.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Stance")
    float StandingCapsuleHalfHeight = 94.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Stance")
    float ProneCapsuleHalfHeight = 48.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Stance")
    float StandingCameraZ = 68.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Stance")
    float ProneCameraZ = 25.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Stance")
    float StanceInterpSpeed = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Stamina", meta=(ClampMin="1.0"))
    float MaxStamina = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Stamina", meta=(ClampMin="0.0"))
    float SprintDrainPerSecond = 24.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Stamina", meta=(ClampMin="0.0"))
    float StaminaRegenPerSecond = 19.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Stamina", meta=(ClampMin="0.0"))
    float StaminaRegenDelay = 0.8f;

    UPROPERTY(VisibleInstanceOnly, Category="Movement|Stamina")
    float Stamina = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category="View|Sway", meta=(ClampMin="0.0"))
    float BaseWeaponSwayCm = 2.4f;

    UPROPERTY(EditDefaultsOnly, Category="View|Recoil", meta=(ClampMin="0.1"))
    float RecoilRecoverySpeed = 8.5f;

    UPROPERTY(EditDefaultsOnly, Category="View|Recoil", meta=(ClampMin="0.0"))
    float RecoilCameraImpulseFOV = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category="View|Suppression", meta=(ClampMin="0.0"))
    float SuppressionFOVPenalty = 1.8f;

    UPROPERTY(EditDefaultsOnly, Category="View|Suppression", meta=(ClampMin="0.0"))
    float SuppressionRollDegrees = 1.2f;

    UPROPERTY(EditDefaultsOnly, Category="View")
    float HipFOV = 78.0f;

    UPROPERTY(EditDefaultsOnly, Category="View")
    float AimFOV = 58.0f;

    UPROPERTY(EditDefaultsOnly, Category="View")
    float AimInterpSpeed = 12.0f;

    bool bAiming = false;
    bool bSprinting = false;
    bool bProne = false;
    bool bBandaging = false;
    float LastSprintActivityTime = -1000.0f;
    float PendingRecoilPitch = 0.0f;
    float PendingRecoilYaw = 0.0f;
    float NextMeleeTime = 0.0f;
    FTimerHandle BandageTimerHandle;

    const FVector HipBodyLocation = FVector(45.0f, 18.0f, -18.0f);
    const FVector AimBodyLocation = FVector(45.0f, 0.0f, -12.5f);
    const FVector HipBarrelLocation = FVector(82.0f, 18.0f, -15.0f);
    const FVector AimBarrelLocation = FVector(82.0f, 0.0f, -10.0f);

    void ApplyRoleLoadout();
    AWPBattlefieldDirector* GetBattlefieldDirector() const;
    bool GetViewTarget(FVector& OutTargetLocation, float MaxRangeCm = 10000.0f) const;

    void StartFireInput();
    void StopFireInput();
    void ReloadInput();
    void MeleeInput();
    void StartBandage();
    void FinishBandage();
    void CancelBandage();

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void JumpInput();
    void ToggleProne();
    bool EnterProne();
    bool ExitProne();
    void StartSprint();
    void StopSprint();
    void StartCrouch();
    void StopCrouch();
    void StartAim();
    void StopAim();
    void ThrowGrenadeInput();
    void ThrowSmokeGrenadeInput();
    void InteractInput();
    void IssueFollowOrder();
    void IssueAttackOrder();
    void IssueHoldOrder();
    void CallArtilleryInput();

    UFUNCTION()
    void HandleShotFired(float RecoilPitch, float RecoilYaw);

    UFUNCTION()
    void HandleDowned();

    UFUNCTION()
    void HandleRevived();

    UFUNCTION()
    void HandleDeath();
};
