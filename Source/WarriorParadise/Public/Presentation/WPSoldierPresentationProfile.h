#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPSoldierPresentationProfile.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class USoundAttenuation;
class USoundBase;

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPSoldierPresentationProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> HitEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> DeathEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> PainSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> DownedSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> ReviveSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> DeathSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> MeleeSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> BandageStartSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> BandageCompleteSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundAttenuation> Attenuation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
    TSoftObjectPtr<UAnimMontage> HitReactMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
    TSoftObjectPtr<UAnimMontage> DownedMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
    TSoftObjectPtr<UAnimMontage> ReviveMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
    TSoftObjectPtr<UAnimMontage> DeathMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
    TSoftObjectPtr<UAnimMontage> MeleeMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
    TSoftObjectPtr<UAnimMontage> BandageMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    FVector HitEffectScale = FVector(0.55f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX")
    FVector DeathEffectScale = FVector::OneVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float Volume = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float PitchRandomization = 0.035f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0"))
    float PainSoundCooldown = 0.35f;
};
