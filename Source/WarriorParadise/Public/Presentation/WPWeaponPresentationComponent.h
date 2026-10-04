#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WPWeaponPresentationComponent.generated.h"

class AWPBallisticProjectile;
class AWPSoldierCharacter;
class UWPWeaponComponent;
class UWPWeaponPresentationProfile;
class USoundAttenuation;
class USoundBase;

UCLASS(ClassGroup=(Presentation), meta=(BlueprintSpawnableComponent))
class WARRIORPARADISE_API UWPWeaponPresentationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWPWeaponPresentationComponent();

    UFUNCTION(BlueprintCallable, Category="Presentation")
    void SetPresentationProfile(UWPWeaponPresentationProfile* NewProfile);

    UFUNCTION(BlueprintPure, Category="Presentation")
    UWPWeaponPresentationProfile* GetPresentationProfile() const { return PresentationProfile; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category="Presentation")
    TObjectPtr<UWPWeaponPresentationProfile> PresentationProfile;

    UPROPERTY(Transient)
    TObjectPtr<USoundAttenuation> RuntimeAttenuation;

    TWeakObjectPtr<AWPSoldierCharacter> SoldierOwner;
    TWeakObjectPtr<UWPWeaponComponent> WeaponComponent;

    void BindWeapon();
    void PlaySound(USoundBase* Sound, const FVector& Location, float VolumeMultiplier) const;

    UFUNCTION()
    void HandleShotFired(float RecoilPitch, float RecoilYaw);

    UFUNCTION()
    void HandleReloadStarted();

    UFUNCTION()
    void HandleReloadCompleted();

    UFUNCTION()
    void HandleReloadCancelled();

    UFUNCTION()
    void HandleDryFire();

    UFUNCTION()
    void HandleProjectileSpawned(AWPBallisticProjectile* Projectile);

    UFUNCTION()
    void HandleProjectileImpact(
        FVector ImpactLocation,
        FVector ImpactNormal,
        AActor* HitActor);
};
