#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPAirSupportVisualProfile.generated.h"

class UNiagaraSystem;
class USoundAttenuation;
class USoundBase;
class UStaticMesh;

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPAirSupportVisualProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aircraft")
    TSoftObjectPtr<UStaticMesh> AircraftMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aircraft")
    FTransform AircraftRelativeTransform = FTransform::Identity;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> GunMuzzleFlash;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> EngineLoop;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> FlybySound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> GunSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> BombReleaseSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundAttenuation> Attenuation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float EngineVolume = 0.72f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float FlybyVolume = 0.95f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float GunVolume = 0.82f;
};
