#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPExplosionPresentationProfile.generated.h"

class UNiagaraSystem;
class USoundAttenuation;
class USoundBase;

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPExplosionPresentationProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> ExplosionEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> ExplosionSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundAttenuation> Attenuation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    FVector EffectScale = FVector::OneVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float Volume = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float PitchRandomization = 0.03f;
};
