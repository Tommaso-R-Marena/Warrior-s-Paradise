#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Pawn.h"
#include "WPLightVehicle.generated.h"

class AWPBattlefieldDirector;
class AWPSoldierCharacter;
class UAudioComponent;
class UBoxComponent;
class UCameraComponent;
class UFloatingPawnMovement;
class UStaticMeshComponent;
class UWPLightVehicleVisualProfile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPLightVehicleSimpleEventSignature);

UCLASS()
class WARRIORPARADISE_API AWPLightVehicle : public APawn
{
    GENERATED_BODY()

public:
    AWPLightVehicle();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual UPawnMovementComponent* GetMovementComponent() const override;

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    void SetCombatTeam(EWPCombatTeam NewTeam) { CombatTeam = NewTeam; }

    UFUNCTION(BlueprintPure, Category="Vehicle")
    EWPCombatTeam GetCombatTeam() const { return CombatTeam; }

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    void SetVehicleType(EWPLightVehicleType NewType);

    UFUNCTION(BlueprintPure, Category="Vehicle")
    EWPLightVehicleType GetVehicleType() const { return VehicleType; }

    UFUNCTION(BlueprintPure, Category="Vehicle")
    bool IsDestroyed() const { return bDestroyed; }

    UFUNCTION(BlueprintPure, Category="Vehicle")
    bool IsOccupied() const { return DriverSoldier.IsValid(); }

    UFUNCTION(BlueprintPure, Category="Vehicle")
    float GetHealthNormalized() const
    {
        return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
    }

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    bool TryEnter(AWPSoldierCharacter* Soldier);

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    void ExitVehicle();

    UFUNCTION(BlueprintCallable, Category="Vehicle|Presentation")
    void SetVisualProfile(UWPLightVehicleVisualProfile* NewProfile)
    {
        VisualProfile = NewProfile;
    }

    UFUNCTION(BlueprintCallable, Category="Vehicle|Presentation")
    void ApplyVisualProfile();

    UPROPERTY(BlueprintAssignable, Category="Vehicle|Presentation")
    FWPLightVehicleSimpleEventSignature OnVehicleDestroyed;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Vehicle")
    TObjectPtr<UBoxComponent> CollisionRoot;

    UPROPERTY(VisibleAnywhere, Category="Vehicle")
    TObjectPtr<UStaticMeshComponent> BodyVisual;

    UPROPERTY(VisibleAnywhere, Category="Vehicle")
    TObjectPtr<UStaticMeshComponent> WeaponVisual;

    UPROPERTY(VisibleAnywhere, Category="Vehicle")
    TObjectPtr<UCameraComponent> DriverCamera;

    UPROPERTY(VisibleAnywhere, Category="Vehicle")
    TObjectPtr<UFloatingPawnMovement> Movement;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> EngineAudioComponent;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Presentation")
    TObjectPtr<UWPLightVehicleVisualProfile> VisualProfile;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle")
    EWPCombatTeam CombatTeam = EWPCombatTeam::Allies;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle")
    EWPLightVehicleType VehicleType = EWPLightVehicleType::TransportTruck;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle")
    float MaxHealth = 420.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Vehicle")
    float Health = 420.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle")
    int32 PassengerCapacity = 5;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle")
    float EmbarkRadiusCm = 850.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Combat")
    float MachineGunIntervalSeconds = 0.12f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Combat")
    float MachineGunRangeCm = 2800.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vehicle|Combat")
    float DesiredCombatRangeCm = 1500.0f;

    bool bDestroyed = false;
    float NextMachineGunTime = 0.0f;
    float NextTargetSearchTime = 0.0f;
    float SteeringInput = 0.0f;

    TWeakObjectPtr<AWPSoldierCharacter> DriverSoldier;
    TArray<TWeakObjectPtr<AWPSoldierCharacter>> Passengers;
    TWeakObjectPtr<AActor> CurrentTarget;
    TWeakObjectPtr<AWPBattlefieldDirector> BattlefieldDirector;

    void MoveForward(float Value);
    void Turn(float Value);
    void FireInput();
    void InteractInput();
    void EmbarkNearbySquad(AWPSoldierCharacter* Driver);
    void DisembarkPassengers(bool bEmergency);
    AActor* FindBestTarget() const;
    void FireMachineGun(AActor* Target);
    void DestroyVehicle();
    void RestoreSoldierFromVehicle(
        AWPSoldierCharacter* Soldier,
        const FVector& ExitLocation,
        bool bEmergency);

    UFUNCTION()
    void HandleTakeAnyDamage(
        AActor* DamagedActor,
        float Damage,
        const UDamageType* DamageType,
        AController* InstigatedBy,
        AActor* DamageCauser);
};
