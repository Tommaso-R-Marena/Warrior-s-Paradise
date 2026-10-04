#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalFogVolume.h"
#include "WPSmokeCloud.generated.h"

class UWPSmokePresentationProfile;

UCLASS()
class WARRIORPARADISE_API AWPSmokeCloud : public ALocalFogVolume
{
    GENERATED_BODY()

public:
    AWPSmokeCloud(const FObjectInitializer& ObjectInitializer);

    void InitializeSmoke(float InRadiusCm = 650.0f, float DurationSeconds = 24.0f);

    UFUNCTION(BlueprintCallable, Category="Smoke|Presentation")
    void SetPresentationProfile(UWPSmokePresentationProfile* NewProfile)
    {
        PresentationProfile = NewProfile;
    }

    UFUNCTION(BlueprintPure, Category="Smoke")
    float GetSmokeRadiusCm() const { return SmokeRadiusCm; }

    bool BlocksSightBetween(const FVector& Start, const FVector& End) const;

private:
    UPROPERTY(EditDefaultsOnly, Category="Smoke|Presentation")
    TObjectPtr<UWPSmokePresentationProfile> PresentationProfile;

    UPROPERTY(EditDefaultsOnly, Category="Smoke", meta=(ClampMin="100.0"))
    float SmokeRadiusCm = 650.0f;
};
