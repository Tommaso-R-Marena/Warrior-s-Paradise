#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPEmplacementVisualProfile.generated.h"

class UNiagaraSystem;
class USoundAttenuation;
class USoundBase;
class UStaticMesh;

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPEmplacementVisualProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Emplacement")
    TSoftObjectPtr<UStaticMesh> BaseMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Emplacement")
    FTransform BaseRelativeTransform = FTransform::Identity;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Emplacement")
    TSoftObjectPtr<UStaticMesh> GunMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Emplacement")
    FTransform GunRelativeTransform = FTransform(
        FRotator::ZeroRotator,
        FVector(0.0f, 0.0f, 85.0f),
        FVector::OneVector);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> MuzzleFlash;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> DestructionEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> FireSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> DestructionSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundAttenuation> Attenuation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float FireVolume = 0.85f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float DestructionVolume = 1.0f;
};
