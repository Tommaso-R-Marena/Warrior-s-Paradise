#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPInfantryLoadoutProfile.generated.h"

class UWPSoldierPresentationProfile;
class UWPInfantryVisualProfile;
class UWPWeaponDefinition;
class UWPWeaponPresentationProfile;

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPInfantryLoadoutProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat")
    TObjectPtr<UWPWeaponDefinition> WeaponDefinition;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UWPInfantryVisualProfile> VisualProfile;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UWPWeaponPresentationProfile> WeaponPresentationProfile;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UWPSoldierPresentationProfile> SoldierPresentationProfile;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment", meta=(ClampMin="0"))
    int32 FragGrenades = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment", meta=(ClampMin="0"))
    int32 SmokeGrenades = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment", meta=(ClampMin="0"))
    int32 Medkits = 1;
};
