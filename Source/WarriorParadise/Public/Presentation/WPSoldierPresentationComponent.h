#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WPSoldierPresentationComponent.generated.h"

class AWPSoldierCharacter;
class UAnimMontage;
class USoundAttenuation;
class USoundBase;
class UWPHealthComponent;
class UWPSoldierPresentationProfile;

UCLASS(ClassGroup=(Presentation), meta=(BlueprintSpawnableComponent))
class WARRIORPARADISE_API UWPSoldierPresentationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWPSoldierPresentationComponent();

    UFUNCTION(BlueprintCallable, Category="Presentation")
    void SetPresentationProfile(UWPSoldierPresentationProfile* NewProfile);

    UFUNCTION(BlueprintPure, Category="Presentation")
    UWPSoldierPresentationProfile* GetPresentationProfile() const
    {
        return PresentationProfile;
    }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category="Presentation")
    TObjectPtr<UWPSoldierPresentationProfile> PresentationProfile;

    UPROPERTY(Transient)
    TObjectPtr<USoundAttenuation> RuntimeAttenuation;

    TWeakObjectPtr<AWPSoldierCharacter> SoldierOwner;
    TWeakObjectPtr<UWPHealthComponent> HealthComponent;
    float NextPainSoundTime = 0.0f;

    void BindEvents();
    void PlayWorldSound(USoundBase* Sound, float VolumeMultiplier = 1.0f) const;
    void PlayMontage(UAnimMontage* Montage) const;
    void SpawnEffectAtSoldier(class UNiagaraSystem* System, const FVector& Scale) const;

    UFUNCTION()
    void HandleDamageReceived(float Damage, AActor* DamageCauser);

    UFUNCTION()
    void HandleDowned();

    UFUNCTION()
    void HandleRevived();

    UFUNCTION()
    void HandleDeath();

    UFUNCTION()
    void HandleMelee();

    UFUNCTION()
    void HandleBandageStarted();

    UFUNCTION()
    void HandleBandageCompleted();
};
