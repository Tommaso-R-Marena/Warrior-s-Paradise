#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPVehicleVisualProfile.generated.h"

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
};
