#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPWeaponPresentationProfile.generated.h"

class UNiagaraSystem;
class USoundBase;
class USoundAttenuation;

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPWeaponPresentationProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> MuzzleFlash;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> BulletImpact;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX|Impact")
    TSoftObjectPtr<UNiagaraSystem> WoodImpact;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX|Impact")
    TSoftObjectPtr<UNiagaraSystem> StoneImpact;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX|Impact")
    TSoftObjectPtr<UNiagaraSystem> MetalImpact;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX|Impact")
    TSoftObjectPtr<UNiagaraSystem> FleshImpact;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> FireSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> ReloadStartSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> ReloadCompleteSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> ReloadCancelSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> DryFireSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> BulletImpactSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Impact")
    TSoftObjectPtr<USoundBase> WoodImpactSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Impact")
    TSoftObjectPtr<USoundBase> StoneImpactSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Impact")
    TSoftObjectPtr<USoundBase> MetalImpactSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Impact")
    TSoftObjectPtr<USoundBase> FleshImpactSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundAttenuation> Attenuation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float FireVolume = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float ImpactVolume = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float PitchRandomization = 0.025f;
};
