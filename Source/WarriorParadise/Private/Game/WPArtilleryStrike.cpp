#include "Game/WPArtilleryStrike.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSuppressionComponent.h"
#include "Components/LightComponent.h"
#include "Engine/PointLight.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Vehicles/WPArmoredVehicle.h"
#include "World/WPDestructibleCover.h"

AWPArtilleryStrike::AWPArtilleryStrike()
{
    PrimaryActorTick.bCanEverTick = false;
    InitialLifeSpan = 15.0f;
}

void AWPArtilleryStrike::InitializeStrike(const FVector& InTargetLocation, EWPCombatTeam InTeam)
{
    TargetLocation = InTargetLocation;
    StrikeTeam = InTeam;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            ShellTimerHandle,
            this,
            &AWPArtilleryStrike::FireImpact,
            ShellIntervalSeconds,
            true,
            InitialDelaySeconds);
    }
}

FVector AWPArtilleryStrike::ChooseImpactLocation() const
{
    const float Angle = FMath::FRandRange(0.0f, UE_TWO_PI);
    const float Radius = ScatterRadiusCm * FMath::Sqrt(FMath::FRand());
    return TargetLocation + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
}

void AWPArtilleryStrike::FireImpact()
{
    if (!GetWorld())
    {
        Destroy();
        return;
    }

    const FVector ImpactLocation = ChooseImpactLocation();

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Soldier = *It;
        if (!Soldier ||
            Soldier->GetCombatTeam() == StrikeTeam ||
            !Soldier->GetHealthComponent() ||
            Soldier->GetHealthComponent()->IsDead())
        {
            continue;
        }

        const float Distance = FVector::Dist(ImpactLocation, Soldier->GetActorLocation());

        if (Distance <= SuppressionRadiusCm && Soldier->GetSuppressionComponent())
        {
            const float SuppressionAlpha = 1.0f - FMath::Clamp(Distance / SuppressionRadiusCm, 0.0f, 1.0f);
            Soldier->GetSuppressionComponent()->AddSuppression(90.0f * SuppressionAlpha);
        }

        if (Distance <= BlastRadiusCm)
        {
            FHitResult OcclusionHit;
            FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WPArtilleryOcclusion), true, this);
            QueryParams.AddIgnoredActor(this);

            const FVector TargetLocation = Soldier->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
            const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
                OcclusionHit,
                ImpactLocation,
                TargetLocation,
                ECC_Visibility,
                QueryParams) &&
                OcclusionHit.GetActor() != Soldier;

            const float DistanceAlpha = 1.0f - FMath::Clamp(Distance / BlastRadiusCm, 0.0f, 1.0f);
            const float CoverScale = bBlocked ? 0.32f : 1.0f;
            const float Damage = MaximumDamage * FMath::Square(DistanceAlpha) * CoverScale;

            UGameplayStatics::ApplyDamage(
                Soldier,
                Damage,
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    for (TActorIterator<AWPArmoredVehicle> It(GetWorld()); It; ++It)
    {
        AWPArmoredVehicle* Vehicle = *It;
        if (!Vehicle ||
            Vehicle->GetCombatTeam() == StrikeTeam ||
            Vehicle->IsDestroyed())
        {
            continue;
        }

        const float Distance = FVector::Dist(ImpactLocation, Vehicle->GetActorLocation());
        if (Distance <= BlastRadiusCm)
        {
            const float Alpha = 1.0f - FMath::Clamp(Distance / BlastRadiusCm, 0.0f, 1.0f);
            UGameplayStatics::ApplyDamage(
                Vehicle,
                MaximumDamage * 1.25f * FMath::Square(Alpha),
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    for (TActorIterator<AWPDestructibleCover> It(GetWorld()); It; ++It)
    {
        AWPDestructibleCover* Cover = *It;
        if (!Cover)
        {
            continue;
        }

        const float Distance = FVector::Dist(ImpactLocation, Cover->GetActorLocation());
        if (Distance <= BlastRadiusCm)
        {
            const float DistanceAlpha = 1.0f - FMath::Clamp(Distance / BlastRadiusCm, 0.0f, 1.0f);
            UGameplayStatics::ApplyDamage(
                Cover,
                MaximumDamage * 1.35f * FMath::Square(DistanceAlpha),
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    if (APointLight* Flash = GetWorld()->SpawnActor<APointLight>(ImpactLocation + FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator))
    {
        Flash->SetLifeSpan(0.18f);
        if (ULightComponent* Light = Flash->GetLightComponent())
        {
            Light->SetIntensity(70000.0f);
            Light->SetAttenuationRadius(1500.0f);
            Light->SetLightColor(FLinearColor(1.0f, 0.55f, 0.22f));
            Light->SetCastShadows(false);
        }
    }

    OnImpact.Broadcast(ImpactLocation);

    ++ShellsFired;
    if (ShellsFired >= ShellCount)
    {
        GetWorld()->GetTimerManager().ClearTimer(ShellTimerHandle);
        SetLifeSpan(1.0f);
    }
}
