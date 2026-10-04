#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPEnvironmentVisualProfile.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UTextureCube;

UENUM(BlueprintType)
enum class EWPEnvironmentSurface : uint8
{
    Ground,
    Plaster,
    Brick,
    Stone,
    Wood,
    Metal,
    Roof,
    Concrete
};

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPEnvironmentVisualProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Materials")
    TSoftObjectPtr<UMaterialInterface> GroundMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Materials")
    TSoftObjectPtr<UMaterialInterface> PlasterMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Materials")
    TSoftObjectPtr<UMaterialInterface> BrickMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Materials")
    TSoftObjectPtr<UMaterialInterface> StoneMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Materials")
    TSoftObjectPtr<UMaterialInterface> WoodMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Materials")
    TSoftObjectPtr<UMaterialInterface> MetalMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Materials")
    TSoftObjectPtr<UMaterialInterface> RoofMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Materials")
    TSoftObjectPtr<UMaterialInterface> ConcreteMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenery")
    TSoftObjectPtr<UStaticMesh> VillageBuildingMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenery")
    FRotator VillageBuildingRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenery")
    FVector VillageBuildingLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenery")
    TSoftObjectPtr<UStaticMesh> TreeMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenery")
    TSoftObjectPtr<UStaticMesh> CrateMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenery")
    TSoftObjectPtr<UStaticMesh> BarrelMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lighting")
    TSoftObjectPtr<UTextureCube> SkyCubemap;

    TSoftObjectPtr<UMaterialInterface> GetMaterial(EWPEnvironmentSurface Surface) const
    {
        switch (Surface)
        {
            case EWPEnvironmentSurface::Ground:
                return GroundMaterial;
            case EWPEnvironmentSurface::Plaster:
                return PlasterMaterial;
            case EWPEnvironmentSurface::Brick:
                return BrickMaterial;
            case EWPEnvironmentSurface::Stone:
                return StoneMaterial;
            case EWPEnvironmentSurface::Wood:
                return WoodMaterial;
            case EWPEnvironmentSurface::Metal:
                return MetalMaterial;
            case EWPEnvironmentSurface::Roof:
                return RoofMaterial;
            case EWPEnvironmentSurface::Concrete:
            default:
                return ConcreteMaterial;
        }
    }
};
