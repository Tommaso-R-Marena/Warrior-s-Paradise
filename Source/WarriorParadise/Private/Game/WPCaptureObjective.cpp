#include "Game/WPCaptureObjective.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"

AWPCaptureObjective::AWPCaptureObjective()
{
    PrimaryActorTick.bCanEverTick = true;

    Zone = CreateDefaultSubobject<USphereComponent>(TEXT("Zone"));
    SetRootComponent(Zone);
    Zone->InitSphereRadius(CaptureRadius);
    Zone->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight"));
    BeaconLight->SetupAttachment(Zone);
    BeaconLight->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
    BeaconLight->SetAttenuationRadius(900.0f);
    BeaconLight->SetIntensity(5500.0f);
    BeaconLight->SetCastShadows(false);

    ObjectiveLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ObjectiveLabel"));
    ObjectiveLabel->SetupAttachment(Zone);
    ObjectiveLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 245.0f));
    ObjectiveLabel->SetHorizontalAlignment(EHTA_Center);
    ObjectiveLabel->SetWorldSize(48.0f);
    ObjectiveLabel->SetTextRenderColor(FColor(230, 220, 150));
}

void AWPCaptureObjective::InitializeObjective(int32 InObjectiveIndex)
{
    ObjectiveIndex = FMath::Max(0, InObjectiveIndex);
    UpdatePresentation();
}

void AWPCaptureObjective::SetObjectiveActive(bool bNewActive)
{
    if (bCaptured)
    {
        bActive = false;
    }
    else
    {
        bActive = bNewActive;
    }

    SetActorTickEnabled(bActive);
    UpdatePresentation();
}

void AWPCaptureObjective::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bActive || bCaptured || !GetWorld())
    {
        return;
    }

    int32 AlliedPresence = 0;
    int32 AxisPresence = 0;
    const float RadiusSquared = FMath::Square(CaptureRadius);

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Soldier = *It;
        if (!Soldier || Soldier->GetHealthComponent()->IsDead())
        {
            continue;
        }

        if (FVector::DistSquared2D(Soldier->GetActorLocation(), GetActorLocation()) > RadiusSquared)
        {
            continue;
        }

        if (Soldier->GetCombatTeam() == EWPCombatTeam::Allies)
        {
            ++AlliedPresence;
        }
        else
        {
            ++AxisPresence;
        }
    }

    const int32 Advantage = AlliedPresence - AxisPresence;
    if (Advantage > 0)
    {
        const float CaptureMultiplier = FMath::Clamp(static_cast<float>(Advantage), 1.0f, 3.0f);
        CaptureProgress += (DeltaSeconds / SoloCaptureSeconds) * CaptureMultiplier;
    }
    else if (Advantage < 0)
    {
        CaptureProgress -= (DeltaSeconds / SoloCaptureSeconds) * 0.55f;
    }
    else if (AlliedPresence == 0)
    {
        CaptureProgress -= (DeltaSeconds / SoloCaptureSeconds) * 0.12f;
    }

    CaptureProgress = FMath::Clamp(CaptureProgress, 0.0f, 1.0f);
    UpdatePresentation();

    if (CaptureProgress >= 1.0f - KINDA_SMALL_NUMBER)
    {
        CaptureProgress = 1.0f;
        bCaptured = true;
        bActive = false;
        SetActorTickEnabled(false);
        UpdatePresentation();
        OnCaptured.Broadcast(this);
    }
}

void AWPCaptureObjective::UpdatePresentation()
{
    if (!BeaconLight || !ObjectiveLabel)
    {
        return;
    }

    if (bCaptured)
    {
        BeaconLight->SetLightColor(FLinearColor(0.18f, 0.8f, 0.24f));
        BeaconLight->SetIntensity(2600.0f);
        ObjectiveLabel->SetText(FText::FromString(
            FString::Printf(TEXT("OBJECTIVE %d SECURED"), ObjectiveIndex + 1)));
        ObjectiveLabel->SetTextRenderColor(FColor(90, 220, 100));
        return;
    }

    if (!bActive)
    {
        BeaconLight->SetLightColor(FLinearColor(0.22f, 0.28f, 0.34f));
        BeaconLight->SetIntensity(850.0f);
        ObjectiveLabel->SetText(FText::FromString(
            FString::Printf(TEXT("OBJECTIVE %d"), ObjectiveIndex + 1)));
        ObjectiveLabel->SetTextRenderColor(FColor(130, 140, 150));
        return;
    }

    BeaconLight->SetLightColor(FLinearColor(0.95f, 0.64f, 0.12f));
    BeaconLight->SetIntensity(5500.0f);

    const int32 Percent = FMath::RoundToInt(CaptureProgress * 100.0f);
    ObjectiveLabel->SetText(FText::FromString(
        FString::Printf(TEXT("CAPTURE %d   %d%%"), ObjectiveIndex + 1, Percent)));
    ObjectiveLabel->SetTextRenderColor(FColor(240, 210, 100));
}
