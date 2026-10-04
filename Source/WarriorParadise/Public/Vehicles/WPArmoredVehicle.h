#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Pawn.h"
#include "WPArmoredVehicle.generated.h"

class AWPTankShell;
class UBoxComponent;
class UFloatingPawnMovement;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPVehicleSimpleEventSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FWPVehicleDamageSignature,
    float,
    Damage,
    float,
    HealthNormalized);

UCLASS()
class WARRIORPARADISE_API AWPArmoredVehicle : public APawn
{
    GENERATED_BODY()

public:
    AWPArmoredVehicle();

    virtual void Tick(float DeltaSeconds) override;
    virtual UPawnMovementComponent* GetMovementComponent() const override;

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    void SetCombatTeam(EWPCombatTeam NewTeam) { CombatTeam = NewTeam; }

    UFUNCTION(BlueprintPure, Category="Vehicle")
    EWPCombatTeam GetCombatTeam() const { return CombatTeam; }

    UFUNCTION(BlueprintPure, Category="Vehicle")
    float GetHealthNormalized() const { return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f; }

    UFUNCTION(BlueprintPure, Category="Vehicle")
    bool IsDestroyed() const { return bDestroyed; }

    UPROPERTY(BlueprintAssignable, Category="Vehicle|Presentation")
    FWPVehicleSimpleEventSignature OnCannonFired;

    UPROPERTY(BlueprintAssignable, Category="Vehicle|Presentation")
    FWPVehicleSimpleEventSignature OnMachineGunFired;

    UPROPERTY(BlueprintAssignable, Category="Vehicle|Presentation")
    FWPVehicleSimpleEventSignature OnVehicleDestroyed;

    UPROPERTY(BlueprintAssignable, Category="Vehicle|Presentation")
    FWPVehicleDamageSignature OnVehicleDamaged;

    UFUNCTION(BlueprintCallable, Category="Vehicle|Armor")
    void ApplyShellImpact(
        float BaseDamage,
        const FVector& ImpactLocation,
        const FVector& ShotDirection);

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

    float NextCannonTime = 0.0f;
    float NextMachineGunTime = 0.0f;
    float NextTargetScanTime = 0.0f;
    bool bDestroyed = false;
    TWeakObjectPtr<AActor> CurrentTarget;
    TWeakObjectPtr<AWPBattlefieldDirector> BattlefieldDirector;

    AActor* FindBestTarget() const;
    bool HasLineOfSightTo(const AActor* Target) const;
    FVector GetTargetAimPoint(const AActor* Target) const;
    void FireCannon(AActor* Target);
    void FireMachineGun(AActor* Target);
    void DestroyVehicle();

    UFUNCTION()
    void HandleTakeAnyDamage(
        AActor* DamagedActor,
        float Damage,
        const UDamageType* DamageType,
        AController* InstigatedBy,
        AActor* DamageCauser);
};
