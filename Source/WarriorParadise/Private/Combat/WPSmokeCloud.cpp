#include "Combat/WPSmokeCloud.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/WPSmokePresentationProfile.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

AWPSmokeCloud::AWPSmokeCloud(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    if (ULocalFogVolumeComponent* Fog = GetComponent())
    {
        Fog->SetRadialFogExtinction(5.5f);
        Fog->SetHeightFogExtinction(2.4f);
        Fog->SetHeightFogFalloff(18.0f);
        Fog->SetHeightFogOffset(-0.12f);
        Fog->SetFogAlbedo(FLinearColor(0.72f, 0.74f, 0.70f, 1.0f));
        Fog->SetFogEmissive(FLinearColor::Black);
        Fog->SetFogPhaseG(0.12f);
    }
}

void AWPSmokeCloud::InitializeSmoke(float InRadiusCm, float DurationSeconds)
{
    SmokeRadiusCm = FMath::Max(100.0f, InRadiusCm);

    // Local fog volumes use a unit-sphere style transform; 100 cm is a practical
    // gameplay scale baseline for this code-only fallback.
    const float UniformScale = SmokeRadiusCm / 100.0f;
    SetActorScale3D(FVector(UniformScale));

    SetLifeSpan(FMath::Max(1.0f, DurationSeconds));

    if (!PresentationProfile)
    {
        return;
    }

    if (UNiagaraSystem* SmokeSystem =
        PresentationProfile->SmokeEffect.LoadSynchronous())
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            SmokeSystem,
            GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true,
            true,
            ENCPoolMethod::AutoRelease,
            true);
    }

    if (USoundBase* DeploySound =
        PresentationProfile->DeploySound.LoadSynchronous())
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            DeploySound,
            GetActorLocation(),
            PresentationProfile->DeployVolume,
            1.0f,
            0.0f,
            PresentationProfile->Attenuation.LoadSynchronous(),
            nullptr,
            nullptr);
    }

    if (USoundBase* Hiss =
        PresentationProfile->HissLoop.LoadSynchronous())
    {
        UGameplayStatics::SpawnSoundAttached(
            Hiss,
            GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true,
            PresentationProfile->HissVolume,
            1.0f,
            0.0f,
            PresentationProfile->Attenuation.LoadSynchronous(),
            nullptr,
            true);
    }
}

bool AWPSmokeCloud::BlocksSightBetween(const FVector& Start, const FVector& End) const
{
    return FMath::PointDistToSegment(GetActorLocation(), Start, End) <= SmokeRadiusCm;
}
