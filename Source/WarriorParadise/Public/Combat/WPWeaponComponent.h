#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WPWeaponComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPWeaponStateChangedSignature);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class WARRIORPARADISE_API UWPWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWPWeaponComponent();

    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FWPWeaponStateChangedSignature OnWeaponStateChanged;

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void Reload();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void Configure(float InDamage, float InRoundsPerMinute, float InSpreadDegrees);

    UFUNCTION(BlueprintPure, Category="Weapon")
    int32 GetAmmoInMagazine() const { return AmmoInMagazine; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    int32 GetReserveAmmo() const { return ReserveAmmo; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    int32 GetMagazineSize() const { return MagazineSize; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    bool IsReloading() const { return bReloading; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Ballistics")
    float Damage = 34.0f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Ballistics")
    float RangeCm = 20000.0f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Ballistics")
    float SpreadDegrees = 0.8f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    float RoundsPerMinute = 540.0f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    bool bAutomatic = true;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    int32 MagazineSize = 30;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    int32 StartingReserveAmmo = 150;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Handling")
    float ReloadDuration = 1.9f;

    UPROPERTY(VisibleInstanceOnly, Category="Weapon")
    int32 AmmoInMagazine = 30;

    UPROPERTY(VisibleInstanceOnly, Category="Weapon")
    int32 ReserveAmmo = 150;

    bool bReloading = false;
    bool bWantsToFire = false;
    float LastFireTime = -1000.0f;

    FTimerHandle FireTimerHandle;
    FTimerHandle ReloadTimerHandle;

    void TryFire();
    void FinishReload();
    bool BuildShot(FVector& OutStart, FVector& OutDirection, AController*& OutController) const;
};
