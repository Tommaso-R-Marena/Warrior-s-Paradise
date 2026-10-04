#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPWeaponDefinition.generated.h"

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPWeaponDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity")
    FName WeaponId = TEXT("rifle_default");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ballistics", meta=(ClampMin="1.0"))
    float Damage = 34.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ballistics", meta=(ClampMin="1000.0"))
    float MuzzleVelocityCmPerSecond = 78000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ballistics", meta=(ClampMin="1000.0"))
    float RangeCm = 60000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Accuracy", meta=(ClampMin="0.0"))
    float HipSpreadDegrees = 1.15f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Accuracy", meta=(ClampMin="0.0"))
    float AimSpreadDegrees = 0.18f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Handling", meta=(ClampMin="60.0"))
    float RoundsPerMinute = 540.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Handling")
    bool bAutomatic = true;

    // Manual-action weapons (bolt-action rifles, pump-action shotguns, etc.)
    // must complete their action cycle before another round can be fired.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Handling")
    bool bRequiresManualCycle = false;

    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category="Handling",
        meta=(ClampMin="0.05", EditCondition="bRequiresManualCycle"))
    float ManualCycleDuration = 0.65f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Handling", meta=(ClampMin="1"))
    int32 MagazineSize = 30;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Handling", meta=(ClampMin="0"))
    int32 StartingReserveAmmo = 150;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Handling", meta=(ClampMin="0"))
    int32 MaximumReserveAmmo = 240;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Handling", meta=(ClampMin="0.1"))
    float ReloadDuration = 1.9f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Handling", meta=(ClampMin="0.0"))
    float RecoilPitchDegrees = 0.95f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Handling", meta=(ClampMin="0.0"))
    float RecoilYawDegrees = 0.32f;
};
