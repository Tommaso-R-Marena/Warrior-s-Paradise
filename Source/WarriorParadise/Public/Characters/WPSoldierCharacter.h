#pragma once

#include "CoreMinimal.h"
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

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintPure, Category="Combat")
    UWPHealthComponent* GetHealthComponent() const { return HealthComponent; }

    UFUNCTION(BlueprintPure, Category="Combat")
    UWPWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="View")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
    TObjectPtr<UWPHealthComponent> HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
    TObjectPtr<UWPWeaponComponent> WeaponComponent;

private:
    UPROPERTY(VisibleAnywhere, Category="View")
    TObjectPtr<UStaticMeshComponent> ViewWeaponBody;

    UPROPERTY(VisibleAnywhere, Category="View")
    TObjectPtr<UStaticMeshComponent> ViewWeaponBarrel;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float WalkSpeed = 470.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float SprintSpeed = 690.0f;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void StartSprint();
    void StopSprint();
    void StartCrouch();
    void StopCrouch();

    UFUNCTION()
    void HandleDeath();
};
