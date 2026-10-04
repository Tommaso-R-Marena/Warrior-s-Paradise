#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Character.h"
#include "WPSoldierCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UWPHealthComponent;
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
    EWPCombatTeam GetCombatTeam() const { return CombatTeam; }

    UFUNCTION(BlueprintCallable, Category="Combat")
    void SetCombatTeam(EWPCombatTeam NewTeam) { CombatTeam = NewTeam; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="View")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
    TObjectPtr<UWPHealthComponent> HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
    TObjectPtr<UWPWeaponComponent> WeaponComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    EWPCombatTeam CombatTeam = EWPCombatTeam::Allies;

private:
    UPROPERTY(VisibleAnywhere, Category="View")
    TObjectPtr<UStaticMeshComponent> ViewWeaponBody;

    UPROPERTY(VisibleAnywhere, Category="View")
    TObjectPtr<UStaticMeshComponent> ViewWeaponBarrel;

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

    UFUNCTION()
    void HandleShotFired(float RecoilPitch, float RecoilYaw);

    UFUNCTION()
    void HandleDeath();
};
