#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Pawn.h"
#include "UI/WPMobileControls.h"
#include "WPArmoredVehicle.generated.h"

class AWPBattlefieldDirector;
class AWPTankShell;
class AWPSoldierCharacter;
class UBoxComponent;
class UCameraComponent;
class UFloatingPawnMovement;
class UStaticMeshComponent;
class UWPVehicleVisualProfile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPVehicleSimpleEventSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FWPVehicleDamageSignature,
    float,
    Damage,
    float,
    HealthNormalized);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FWPVehicleSubsystemDamageSignature,
    EWPVehicleSubsystem,
    Subsystem,
    float,
    HealthNormalized);

UCLASS()
class WARRIORPARADISE_API AWPArmoredVehicle : public APawn
{
    GENERATED_BODY()

public:
    AWPArmoredVehicle();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // Native Android/iOS gestures reuse established vehicle mechanics.
    void ApplyMobileMovement(float Forward, float Steering);
    void ApplyMobileLook(float Yaw, float Pitch);
    void ExecuteMobileAction(EWPMobileButton Action, bool bPressed);
    virtual UPawnMovementComponent* GetMovementComponent() const override;

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    void SetCombatTeam(EWPCombatTeam NewTeam) { CombatTeam = NewTeam; }

    UFUNCTION(BlueprintPure, Category="Vehicle")
    EWPCombatTeam GetCombatTeam() const { return CombatTeam; }

    UFUNCTION(BlueprintPure, Category="Vehicle")
    float GetHealthNormalized() const { return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f; }

    UFUNCTION(BlueprintPure, Category="Vehicle")
    bool IsDestroyed() const { return bDestroyed; }

    UFUNCTION(BlueprintPure, Category="Vehicle")
    bool IsOccupied() const { return CrewSoldier.IsValid(); }

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    bool TryEnter(AWPSoldierCharacter* Soldier);

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    void ExitVehicle();

    UFUNCTION(BlueprintPure, Category="Vehicle|Combat")
    EWPArmorShellType GetSelectedShellType() const { return SelectedShellType; }

    UFUNCTION(BlueprintPure, Category="Vehicle|Presentation")
    bool HasProductionVisuals() const { return bProductionVisualsActive; }

    UFUNCTION(BlueprintCallable, Category="Vehicle|Presentation")
    void SetVisualProfile(UWPVehicleVisualProfile* NewProfile)
    {
        VisualProfile = NewProfile;
    }

    UFUNCTION(BlueprintCallable, Category="Vehicle|Presentation")
    void ApplyVisualProfile();

    UPROPERTY(BlueprintAssignable, Category="Vehicle|Presentation")
    FWPVehicleSimpleEventSignature OnCannonFired;

    UPROPERTY(BlueprintAssignable, Category="Vehicle|Presentation")
    FWPVehicleSimpleEventSignature OnMachineGunFired;

    UPROPERTY(BlueprintAssignable, Category="Vehicle|Presentation")
    FWPVehicleSimpleEventSignature OnVehicleDestroyed;

    UPROPERTY(BlueprintAssignable, Category="Vehicle|Presentation")
    FWPVehicleDamageSignature OnVehicleDamaged;

    UPROPERTY(BlueprintAssignable, Category="Vehicle|Presentation")
    FWPVehicleSubsystemDamageSignature OnSubsystemDamaged;

    UFUNCTION(BlueprintCallable, Category="Vehicle|Armor")
    void ApplyShellImpact(
        float BaseDamage,
        const FVector& ImpactLocation,
        const FVector& ShotDirection,
        EWPArmorShellType ShellType);

    UFUNCTION(BlueprintPure, Category="Vehicle|Armor")
    float GetSubsystemHealthNormalized(EWPVehicleSubsystem Subsystem) const;

    UFUNCTION(BlueprintPure, Category="Vehicle|Armor")
    bool IsImmobilized() const
    {
        return EngineHealth <= 0.0f || TrackHealth <= 0.0f;
    }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Vehicle")
    TObjectPtr<UBoxComponent> CollisionRoot;

    UPROPERTY(VisibleAnywhere, Category="Vehicle")
    TObjectPtr<UStaticMeshComponent> HullVisual;

    UPROPERTY(VisibleAnywhere, Category="Vehicle")
    TObjectPtr<UStaticMeshComponent> TurretVisual;

    UPROPERTY(VisibleAnywhere, Category="Vehicle")
    TObjectPtr<UStaticMeshComponent> BarrelVisual;

    UPROPERTY(VisibleAnywhere, Category="Vehicle")
    TObjectPtr<UFloatingPawnMovement> Movement;

    UPROPERTY(VisibleAnywhere, Category="Vehicle")
    TObjectPtr<UCameraComponent> CrewCamera;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Presentation")
    TObjectPtr<UWPVehicleVisualProfile> VisualProfile;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle")
    TSubclassOf<AWPTankShell> TankShellClass;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle")
    EWPCombatTeam CombatTeam = EWPCombatTeam::Axis;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle")
    float MaxHealth = 950.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Vehicle")
    float Health = 950.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Combat")
    float CannonCooldownSeconds = 4.4f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Combat")
    float MachineGunIntervalSeconds = 0.18f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Combat")
    float CannonRangeCm = 6500.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Combat")
    float MachineGunRangeCm = 3000.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Combat")
    float DesiredVehicleRangeCm = 2300.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Subsystems")
    float EngineMaxHealth = 260.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Subsystems")
    float TrackMaxHealth = 220.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Subsystems")
    float TurretMaxHealth = 240.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Subsystems")
    float AmmoRackMaxHealth = 180.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Subsystems")
    float AmmoRackCatastrophicDamage = 620.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Vehicle|Subsystems")
    float EngineHealth = 260.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Vehicle|Subsystems")
    float TrackHealth = 220.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Vehicle|Subsystems")
    float TurretHealth = 240.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Vehicle|Subsystems")
    float AmmoRackHealth = 180.0f;

    float NextCannonTime = 0.0f;
    float NextMachineGunTime = 0.0f;
    float PlayerTurretYaw = 0.0f;
    float PlayerGunPitch = 0.0f;
    bool bPlayerMachineGunFiring = false;
    EWPArmorShellType SelectedShellType = EWPArmorShellType::ArmorPiercing;
    float NextTargetScanTime = 0.0f;
    bool bDestroyed = false;
    bool bProductionVisualsActive = false;
    TWeakObjectPtr<AActor> CurrentTarget;
    TWeakObjectPtr<AWPSoldierCharacter> CrewSoldier;
    TWeakObjectPtr<AWPBattlefieldDirector> BattlefieldDirector;

    AActor* FindBestTarget() const;
    bool HasLineOfSightTo(const AActor* Target) const;
    FVector GetTargetAimPoint(const AActor* Target) const;
    void FireCannon(AActor* Target);
    void FireCannonAt(
        const FVector& TargetLocation,
        EWPArmorShellType ShellType);
    void FireMachineGun(AActor* Target);
    void FireMachineGunAt(const FVector& TargetLocation);

    void MoveForwardInput(float Value);
    void TurnHullInput(float Value);
    void TurnTurretInput(float Value);
    void PitchGunInput(float Value);
    void PlayerFireCannon();
    void StartPlayerMachineGun();
    void StopPlayerMachineGun();
    void ToggleShellType();
    void InteractInput();
    FVector GetPlayerAimPoint(float RangeCm) const;
    void RestoreCrewSoldier(bool bEmergency);
    void DamageSubsystem(
        EWPVehicleSubsystem Subsystem,
        float Damage);
    void UpdateSubsystemEffects();
    void DestroyVehicle();

    UFUNCTION()
    void HandleTankShellImpact(FVector ImpactLocation);

    UFUNCTION()
    void HandleTakeAnyDamage(
        AActor* DamagedActor,
        float Damage,
        const UDamageType* DamageType,
        AController* InstigatedBy,
        AActor* DamageCauser);
};
