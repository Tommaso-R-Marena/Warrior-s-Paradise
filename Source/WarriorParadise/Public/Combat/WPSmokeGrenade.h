#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WPSmokeGrenade.generated.h"

class AWPSmokeCloud;
class USphereComponent;
class UProjectileMovementComponent;
class UWPSmokePresentationProfile;

UCLASS()
class WARRIORPARADISE_API AWPSmokeGrenade : public AActor
{
    GENERATED_BODY()

public:
    AWPSmokeGrenade();

    void InitializeSmokeGrenade(const FVector& LaunchVelocity);

    UFUNCTION(BlueprintCallable, Category="Smoke|Presentation")
    void SetPresentationProfile(UWPSmokePresentationProfile* NewProfile)
    {
        PresentationProfile = NewProfile;
    }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Smoke")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category="Smoke")
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY(EditDefaultsOnly, Category="Smoke|Presentation")
    TObjectPtr<UWPSmokePresentationProfile> PresentationProfile;

    UPROPERTY(EditDefaultsOnly, Category="Smoke")
    TSubclassOf<AWPSmokeCloud> SmokeCloudClass;

    UPROPERTY(EditDefaultsOnly, Category="Smoke", meta=(ClampMin="0.1"))
    float FuseSeconds = 2.4f;

    UPROPERTY(EditDefaultsOnly, Category="Smoke", meta=(ClampMin="100.0"))
    float SmokeRadiusCm = 650.0f;

    UPROPERTY(EditDefaultsOnly, Category="Smoke", meta=(ClampMin="1.0"))
    float SmokeDurationSeconds = 24.0f;

    FTimerHandle FuseTimerHandle;

    void DeploySmoke();
};
