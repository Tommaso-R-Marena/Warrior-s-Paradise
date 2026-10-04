#include "Vehicles/WPAirSupportAircraft.h"
#include "Combat/WPBallisticProjectile.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/WPAirSupportVisualProfile.h"
#include "Presentation/WPExplosionPresentationProfile.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/WPAirBomb.h"

AWPAirSupportAircraft::AWPAirSupportAircraft()
{
    PrimaryActorTick.bCanEverTick = true;

    AircraftVisual = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("AircraftVisual"));
    SetRootComponent(AircraftVisual);
    AircraftVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(
        TEXT("/Engine/BasicShapes/Cone.Cone"));
    if (ConeAsset.Succeeded())
    {
        AircraftVisual->SetStaticMesh(ConeAsset.Object);
        AircraftVisual->SetRelativeScale3D(FVector(1.2f, 0.55f, 0.22f));
        AircraftVisual->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    }
}

void AWPAirSupportAircraft::BeginPlay()
{
    Super::BeginPlay();
}

void AWPAirSupportAircraft::InitializeMission(
    EWPCombatTeam InTeam,
    EWPAirSupportMission InMission,
    const FVector& InTargetLocation,
    UWPAirSupportVisualProfile* InVisualProfile,
    UWPExplosionPresentationProfile* InBombExplosionProfile)
{
    CombatTeam = InTeam;
    Mission = InMission;
    TargetLocation = InTargetLocation;
    VisualProfile = InVisualProfile;
    BombExplosionProfile = InBombExplosionProfile;

    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;

    if (Direction.IsNearlyZero())
    {
        Direction = GetActorForwardVector();
    }

    FlightDirection = Direction.GetSafeNormal();
    SetActorRotation(FlightDirection.Rotation());
    EndLocation =
        TargetLocation +
        FlightDirection * 7200.0f +
        FVector(0.0f, 0.0f, AttackAltitudeCm);

    if (VisualProfile)
    {
        if (UStaticMesh* Mesh =
            VisualProfile->AircraftMesh.LoadSynchronous())
        {
            AircraftVisual->SetStaticMesh(Mesh);
            AircraftVisual->SetRelativeTransform(
                VisualProfile->AircraftRelativeTransform);
        }

        if (USoundBase* Engine =
            VisualProfile->EngineLoop.LoadSynchronous())
        {
            EngineAudioComponent = UGameplayStatics::SpawnSoundAttached(
                Engine,
                AircraftVisual,
                NAME_None,
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                EAttachLocation::KeepRelativeOffset,
                true,
                VisualProfile->EngineVolume,
                1.0f,
                0.0f,
                VisualProfile->Attenuation.LoadSynchronous(),
                nullptr,
                false);
        }

        PlayOneShot(
            VisualProfile->FlybySound.LoadSynchronous(),
            VisualProfile->FlybyVolume);
    }
}

void AWPAirSupportAircraft::PlayOneShot(
    USoundBase* Sound,
    float Volume) const
{
    if (!Sound || !VisualProfile)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(
        this,
        Sound,
        GetActorLocation(),
        Volume,
        FMath::FRandRange(0.98f, 1.02f),
        0.0f,
        VisualProfile->Attenuation.LoadSynchronous(),
        nullptr,
        nullptr);
}

void AWPAirSupportAircraft::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const FVector NewLocation =
        GetActorLocation() +
        FlightDirection * FlightSpeedCmPerSecond * DeltaSeconds;
    SetActorLocation(NewLocation);

    if (!bAttackStarted &&
        Mission != EWPAirSupportMission::Flyby &&
        FVector::DistSquared2D(NewLocation, TargetLocation) <=
            FMath::Square(1500.0f))
    {
        BeginAttack();
    }

    const float PassedTarget =
        FVector::DotProduct(
            NewLocation - TargetLocation,
            FlightDirection);

    if (PassedTarget >= 7200.0f)
    {
        CompleteMission();
    }
}

void AWPAirSupportAircraft::BeginAttack()
{
    if (bAttackStarted || !GetWorld())
    {
        return;
    }

    bAttackStarted = true;
    OnAttackStarted.Broadcast();

    if (Mission == EWPAirSupportMission::Strafe)
    {
        RemainingAttackUnits = StrafeRounds;
        GetWorld()->GetTimerManager().SetTimer(
            AttackTimer,
            this,
            &AWPAirSupportAircraft::FireStrafeRound,
            StrafeIntervalSeconds,
            true,
            0.0f);
    }
    else if (Mission == EWPAirSupportMission::Bombing)
    {
        RemainingAttackUnits = BombCount;
        GetWorld()->GetTimerManager().SetTimer(
            AttackTimer,
            this,
            &AWPAirSupportAircraft::DropBomb,
            BombIntervalSeconds,
            true,
            0.0f);
    }
}

void AWPAirSupportAircraft::FireStrafeRound()
{
    if (!GetWorld() || RemainingAttackUnits <= 0)
    {
        FinishAttack();
        return;
    }

    FVector AimPoint =
        TargetLocation +
        FVector(
            FMath::FRandRange(-520.0f, 520.0f),
            FMath::FRandRange(-520.0f, 520.0f),
            40.0f);

    const FVector Muzzle =
        GetActorLocation() +
        FlightDirection * 140.0f -
        FVector(0.0f, 0.0f, 25.0f);
    const FVector Direction =
        (AimPoint - Muzzle).GetSafeNormal();

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AWPBallisticProjectile* Projectile =
        GetWorld()->SpawnActor<AWPBallisticProjectile>(
            AWPBallisticProjectile::StaticClass(),
            Muzzle,
            Direction.Rotation(),
            SpawnParameters))
    {
        Projectile->InitializeProjectile(
            FMath::VRandCone(
                Direction,
                FMath::DegreesToRadians(0.65f)),
            92000.0f,
            30.0f,
            16000.0f,
            nullptr,
            CombatTeam);
    }

    if (VisualProfile)
    {
        if (UNiagaraSystem* System =
            VisualProfile->GunMuzzleFlash.LoadSynchronous())
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                this,
                System,
                Muzzle,
                Direction.Rotation());
        }

        PlayOneShot(
            VisualProfile->GunSound.LoadSynchronous(),
            VisualProfile->GunVolume);
    }

    --RemainingAttackUnits;
    if (RemainingAttackUnits <= 0)
    {
        FinishAttack();
    }
}

void AWPAirSupportAircraft::DropBomb()
{
    if (!GetWorld() || RemainingAttackUnits <= 0)
    {
        FinishAttack();
        return;
    }

    const FVector ReleaseLocation =
        GetActorLocation() -
        FVector(0.0f, 0.0f, 45.0f);

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AWPAirBomb* Bomb = GetWorld()->SpawnActor<AWPAirBomb>(
        AWPAirBomb::StaticClass(),
        ReleaseLocation,
        GetActorRotation(),
        SpawnParameters))
    {
        Bomb->InitializeBomb(
            CombatTeam,
            FlightDirection * 1100.0f +
                FVector(0.0f, 0.0f, -280.0f),
            BombExplosionProfile);
    }

    if (VisualProfile)
    {
        PlayOneShot(
            VisualProfile->BombReleaseSound.LoadSynchronous(),
            0.72f);
    }

    --RemainingAttackUnits;
    if (RemainingAttackUnits <= 0)
    {
        FinishAttack();
    }
}

void AWPAirSupportAircraft::FinishAttack()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(AttackTimer);
    }
}

void AWPAirSupportAircraft::CompleteMission()
{
    if (IsActorBeingDestroyed())
    {
        return;
    }

    FinishAttack();

    if (EngineAudioComponent)
    {
        EngineAudioComponent->Stop();
    }

    OnMissionCompleted.Broadcast();
    Destroy();
}
