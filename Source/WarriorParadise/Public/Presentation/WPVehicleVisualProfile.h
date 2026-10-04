#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPVehicleVisualProfile.generated.h"

class UNiagaraSystem;
class USoundAttenuation;
class USoundBase;
class UStaticMesh;

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPVehicleVisualProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    TSoftObjectPtr<UStaticMesh> HullMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    FTransform HullRelativeTransform = FTransform::Identity;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    TSoftObjectPtr<UStaticMesh> TurretMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    FTransform TurretRelativeTransform = FTransform(
        FRotator::ZeroRotator,
        FVector(-15.0f, 0.0f, 115.0f),
        FVector::OneVector);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    TSoftObjectPtr<UStaticMesh> BarrelMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    FTransform BarrelRelativeTransform = FTransform(
        FRotator(0.0f, 90.0f, 0.0f),
        FVector(170.0f, 0.0f, 120.0f),
        FVector::OneVector);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    bool bHideFallbackTurretWhenMissing = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle")
    bool bHideFallbackBarrelWhenMissing = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> CannonMuzzleFlash;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> MachineGunMuzzleFlash;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> ShellImpactEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> DestructionEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> CannonSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> MachineGunSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> ShellImpactSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> DestructionSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundAttenuation> Attenuation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    FVector CannonMuzzleScale = FVector::OneVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    FVector MachineGunMuzzleScale = FVector(0.55f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    FVector ShellImpactScale = FVector(1.4f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    FVector DestructionScale = FVector(2.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float CannonVolume = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float MachineGunVolume = 0.78f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float ImpactVolume = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float DestructionVolume = 1.0f;
};
