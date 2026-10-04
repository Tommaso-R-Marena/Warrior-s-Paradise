#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPSmokePresentationProfile.generated.h"

class UNiagaraSystem;
class USoundAttenuation;
class USoundBase;

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPSmokePresentationProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> SmokeEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> DeploySound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> HissLoop;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundAttenuation> Attenuation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    FVector EffectScale = FVector(1.35f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float DeployVolume = 0.75f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float HissVolume = 0.42f;
};
