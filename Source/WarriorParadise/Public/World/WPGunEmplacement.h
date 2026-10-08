#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Pawn.h"
#include "UI/WPMobileControls.h"
#include "WPGunEmplacement.generated.h"

class AWPSoldierCharacter;
class UBoxComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UWPEmplacementVisualProfile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPEmplacementSimpleEventSignature);

UCLASS()
class WARRIORPARADISE_API AWPGunEmplacement : public APawn
{
    GENERATED_BODY()

public:
    AWPGunEmplacement();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // Native Android/iOS gestures reuse established vehicle mechanics.
    void ApplyMobileMovement(float Forward, float Steering);
    void ApplyMobileLook(float Yaw, float Pitch);
    void ExecuteMobileAction(EWPMobileButton Action, bool bPressed);

    UFUNCTION(BlueprintCallable, Category="Emplacement")
    void Configure(
        EWPCombatTeam InTeam,
        EWPEmplacementType InType);

    UFUNCTION(BlueprintPure, Category="Emplacement")
    EWPCombatTeam GetCombatTeam() const { return CombatTeam; }

    UFUNCTION(BlueprintPure, Category="Emplacement")
    EWPEmplacementType GetEmplacementType() const { return EmplacementType; }

    UFUNCTION(BlueprintPure, Category="Emplacement")
    bool IsDestroyed() const { return bDestroyed; }

    UFUNCTION(BlueprintPure, Category="Emplacement")
    bool IsOccupied() const { return OperatorSoldier.IsValid(); }

    UFUNCTION(BlueprintPure, Category="Emplacement")
    float GetHealthNormalized() const
    {
        return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
    }

    UFUNCTION(BlueprintCallable, Category="Emplacement")
    bool TryUse(AWPSoldierCharacter* Soldier);

    UFUNCTION(BlueprintCallable, Category="Emplacement")
    void ExitEmplacement();

    UFUNCTION(BlueprintCallable, Category="Emplacement|Presentation")
    void SetVisualProfile(UWPEmplacementVisualProfile* NewProfile)
    {
        VisualProfile = NewProfile;
    }

    UFUNCTION(BlueprintCallable, Category="Emplacement|Presentation")
    void ApplyVisualProfile();

    UPROPERTY(BlueprintAssignable, Category="Emplacement|Presentation")
    FWPEmplacementSimpleEventSignature OnFired;

    UPROPERTY(BlueprintAssignable, Category="Emplacement|Presentation")
    FWPEmplacementSimpleEventSignature OnDestroyed;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Emplacement")
    TObjectPtr<UBoxComponent> CollisionRoot;

    UPROPERTY(VisibleAnywhere, Category="Emplacement")
    TObjectPtr<UStaticMeshComponent> BaseVisual;

    UPROPERTY(VisibleAnywhere, Category="Emplacement")
    TObjectPtr<UStaticMeshComponent> GunVisual;

    UPROPERTY(VisibleAnywhere, Category="Emplacement")
    TObjectPtr<UCameraComponent> GunCamera;

    UPROPERTY(EditDefaultsOnly, Category="Emplacement|Presentation")
    TObjectPtr<UWPEmplacementVisualProfile> VisualProfile;

    UPROPERTY(EditDefaultsOnly, Category="Emplacement")
    EWPCombatTeam CombatTeam = EWPCombatTeam::Allies;

    UPROPERTY(EditDefaultsOnly, Category="Emplacement")
    EWPEmplacementType EmplacementType = EWPEmplacementType::HeavyMachineGun;

    UPROPERTY(EditDefaultsOnly, Category="Emplacement")
    float MaxHealth = 340.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Emplacement")
    float Health = 340.0f;

    UPROPERTY(EditDefaultsOnly, Category="Emplacement|Combat")
    float HMGFireInterval = 0.095f;

    UPROPERTY(EditDefaultsOnly, Category="Emplacement|Combat")
    float ATFireInterval = 4.8f;

    UPROPERTY(EditDefaultsOnly, Category="Emplacement|Combat")
    float HMGRangeCm = 4200.0f;

    UPROPERTY(EditDefaultsOnly, Category="Emplacement|Combat")
    float ATRangeCm = 7200.0f;

    UPROPERTY(EditDefaultsOnly, Category="Emplacement|Combat")
    float MaximumYawDegrees = 78.0f;

    UPROPERTY(EditDefaultsOnly, Category="Emplacement|Combat")
    float MinimumPitchDegrees = -12.0f;

    UPROPERTY(EditDefaultsOnly, Category="Emplacement|Combat")
    float MaximumPitchDegrees = 28.0f;

    float NextFireTime = 0.0f;
    float NextTargetScanTime = 0.0f;
    float LocalYaw = 0.0f;
    float LocalPitch = 0.0f;
    bool bPlayerFiring = false;
    bool bDestroyed = false;

    TWeakObjectPtr<AWPSoldierCharacter> OperatorSoldier;
    TWeakObjectPtr<AActor> CurrentTarget;

    void Turn(float Value);
    void LookUp(float Value);
    void StartFire();
    void StopFire();
    void InteractInput();
    void FireAt(const FVector& TargetLocation, AActor* TargetActor);
    AActor* FindBestTarget() const;
    bool HasLineOfSightTo(const AActor* Target) const;
    bool CanAimAt(const FVector& TargetLocation) const;
    void UpdateGunRotation();
    void DestroyEmplacement();

    UFUNCTION()
    void HandleTakeAnyDamage(
        AActor* DamagedActor,
        float Damage,
        const UDamageType* DamageType,
        AController* InstigatedBy,
        AActor* DamageCauser);
};
