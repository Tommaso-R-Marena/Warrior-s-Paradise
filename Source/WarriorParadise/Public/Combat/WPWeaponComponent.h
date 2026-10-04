#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WPWeaponComponent.generated.h"

class AController;
class AWPBallisticProjectile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPWeaponStateChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWPShotFiredSignature, float, RecoilPitch, float, RecoilYaw);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class WARRIORPARADISE_API UWPWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWPWeaponComponent();

    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FWPWeaponStateChangedSignature OnWeaponStateChanged;

    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FWPShotFiredSignature OnShotFired;

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void Reload();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void SetAiming(bool bNewAiming);

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void Configure(float InDamage, float InRoundsPerMinute, float InSpreadDegrees);

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void AddReserveAmmo(int32 Amount);

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void SetAccuracyMultiplier(float Multiplier);

    UFUNCTION(BlueprintPure, Category="Weapon")
    int32 GetAmmoInMagazine() const { return AmmoInMagazine; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    int32 GetReserveAmmo() const { return ReserveAmmo; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    int32 GetMagazineSize() const { return MagazineSize; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    bool IsReloading() const { return bReloading; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    bool IsAiming() const { return bAiming; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Ballistics")
    TSubclassOf<AWPBallisticProjectile> ProjectileClass;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Ballistics")
    float Damage = 34.0f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Ballistics", meta=(ClampMin="1000.0"))
    float MuzzleVelocityCmPerSecond = 78000.0f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Ballistics", meta=(ClampMin="1000.0"))
    float RangeCm = 60000.0f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Accuracy", meta=(ClampMin="0.0"))
    float HipSpreadDegrees = 1.15f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Accuracy", meta=(ClampMin="0.0"))
    float AimSpreadDegrees = 0.18f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    float RoundsPerMinute = 540.0f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    bool bAutomatic = true;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    int32 MagazineSize = 30;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    int32 StartingReserveAmmo = 150;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    int32 MaximumReserveAmmo = 240;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    float ReloadDuration = 1.9f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    float RecoilPitchDegrees = 0.95f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    float RecoilYawDegrees = 0.32f;

    UPROPERTY(VisibleInstanceOnly, Category="Weapon")
    int32 AmmoInMagazine = 30;

    UPROPERTY(VisibleInstanceOnly, Category="Weapon")
    int32 ReserveAmmo = 150;

    bool bReloading = false;
    bool bWantsToFire = false;
    bool bAiming = false;
    float AccuracyMultiplier = 1.0f;
    float LastFireTime = -1000.0f;

    FTimerHandle FireTimerHandle;
    FTimerHandle ReloadTimerHandle;

    void TryFire();
    void FinishReload();
    bool BuildShot(FVector& OutStart, FVector& OutDirection, AController*& OutController) const;
};
