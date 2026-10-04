#include "Combat/WPSmokeCloud.h"
#include "Components/LocalFogVolumeComponent.h"

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
}

bool AWPSmokeCloud::BlocksSightBetween(const FVector& Start, const FVector& End) const
{
    return FMath::PointDistToSegment(GetActorLocation(), Start, End) <= SmokeRadiusCm;
}
