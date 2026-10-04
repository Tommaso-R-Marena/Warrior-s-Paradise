#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPLightVehicleVisualProfile.generated.h"

class UNiagaraSystem;
class USoundAttenuation;
class USoundBase;
class UStaticMesh;

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPLightVehicleVisualProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    TSoftObjectPtr<UStaticMesh> BodyMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    FTransform BodyRelativeTransform = FTransform::Identity;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    TSoftObjectPtr<UStaticMesh> WeaponMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    FTransform WeaponRelativeTransform = FTransform(
        FRotator::ZeroRotator,
        FVector(45.0f, 0.0f, 115.0f),
        FVector::OneVector);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> MachineGunMuzzleFlash;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> DestructionEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> EngineLoop;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> MachineGunSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> DestructionSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundAttenuation> Attenuation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float EngineVolume = 0.7f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float WeaponVolume = 0.82f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float DestructionVolume = 1.0f;
};
