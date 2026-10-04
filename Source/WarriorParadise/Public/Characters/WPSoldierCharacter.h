#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Character.h"
#include "WPSoldierCharacter.generated.h"

class AWPBattlefieldDirector;
class AWPFragGrenade;
class UCameraComponent;
class UStaticMeshComponent;
class UWPHealthComponent;
class UWPSuppressionComponent;
class UWPWeaponComponent;

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

    UFUNCTION(BlueprintCallable, Category="Combat")
    bool ThrowGrenadeAt(const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category="Combat")
    bool TryReviveSoldier(AWPSoldierCharacter* Target);

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

    UPROPERTY(EditDefaultsOnly, Category="Combat|Grenade", meta=(ClampMin="0"))
    int32 GrenadesRemaining = 2;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Grenade", meta=(ClampMin="100.0"))
    float GrenadeThrowSpeed = 1350.0f;

    UPROPERTY(EditDefaultsOnly, Category="Combat|Interaction", meta=(ClampMin="50.0"))
    float ReviveInteractionRange = 225.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float WalkSpeed = 470.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float SprintSpeed = 690.0f;

    UPROPERTY(EditDefaultsOnly, Category="View")
    float HipFOV = 78.0f;

    UPROPERTY(EditDefaultsOnly, Category="View")
    float AimFOV = 58.0f;

    UPROPERTY(EditDefaultsOnly, Category="View")
    float AimInterpSpeed = 12.0f;

    bool bAiming = false;

    const FVector HipBodyLocation = FVector(45.0f, 18.0f, -18.0f);
    const FVector AimBodyLocation = FVector(45.0f, 0.0f, -12.5f);
    const FVector HipBarrelLocation = FVector(82.0f, 18.0f, -15.0f);
    const FVector AimBarrelLocation = FVector(82.0f, 0.0f, -10.0f);

    void ApplyRoleLoadout();
    AWPBattlefieldDirector* GetBattlefieldDirector() const;
    bool GetViewTarget(FVector& OutTargetLocation, float MaxRangeCm = 10000.0f) const;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void StartSprint();
    void StopSprint();
    void StartCrouch();
    void StopCrouch();
    void StartAim();
    void StopAim();
    void ThrowGrenadeInput();
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
