#include "Game/WPBattlefieldDirector.h"
#include "AI/WPCoverPoint.h"
#include "AI/WPEnemySoldier.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/WPArtilleryStrike.h"
#include "Game/WPCaptureObjective.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/WPArmoredVehicle.h"
#include "World/WPDestructibleCover.h"

AWPBattlefieldDirector::AWPBattlefieldDirector()
{
    PrimaryActorTick.bCanEverTick = false;

    AISoldierClass = AWPEnemySoldier::StaticClass();
    ObjectiveClass = AWPCaptureObjective::StaticClass();
    CoverPointClass = AWPCoverPoint::StaticClass();
    ArtilleryStrikeClass = AWPArtilleryStrike::StaticClass();
    ArmoredVehicleClass = AWPArmoredVehicle::StaticClass();

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeAsset.Succeeded())
    {
        CubeMesh = CubeAsset.Object;
    }
}

void AWPBattlefieldDirector::BeginPlay()
{
    Super::BeginPlay();

    BuildBattlefield();
    BuildCoverPoints();

    AWPSoldierCharacter* Player = Cast<AWPSoldierCharacter>(
        UGameplayStatics::GetPlayerCharacter(this, 0));

    if (Player)
    {
        Player->SetCombatTeam(EWPCombatTeam::Allies);
        Player->SetSoldierRole(EWPSoldierRole::Rifleman);
        Player->GetHealthComponent()->SetCanBeDowned(true);

        const FRotator StartRotation(0.0f, -90.0f, 0.0f);
        Player->SetActorLocationAndRotation(
            FVector(0.0f, 3200.0f, 140.0f),
            StartRotation,
            false,
            nullptr,
            ETeleportType::TeleportPhysics);

        if (AController* Controller = Player->GetController())
        {
            Controller->SetControlRotation(StartRotation);
        }

        FriendlySoldiersAlive = 1;
    }

    BuildObjectives();
    SquadOrderLocation = GetActiveObjectiveLocation();
    SpawnFriendlySquad();
    SpawnWave();
}

int32 AWPBattlefieldDirector::GetActiveObjectiveNumber() const
{
    return Objectives.IsValidIndex(ActiveObjectiveIndex) ? ActiveObjectiveIndex + 1 : 0;
}

float AWPBattlefieldDirector::GetActiveObjectiveProgress() const
{
    if (Objectives.IsValidIndex(ActiveObjectiveIndex) && Objectives[ActiveObjectiveIndex])
    {
        return Objectives[ActiveObjectiveIndex]->GetCaptureProgress();
    }

    return bBattleWon ? 1.0f : 0.0f;
}

FVector AWPBattlefieldDirector::GetActiveObjectiveLocation() const
{
    if (Objectives.IsValidIndex(ActiveObjectiveIndex) && Objectives[ActiveObjectiveIndex])
    {
        return Objectives[ActiveObjectiveIndex]->GetActorLocation();
    }

    return FVector::ZeroVector;
}

float AWPBattlefieldDirector::GetArtilleryCooldownRemaining() const
{
    if (!GetWorld())
    {
        return 0.0f;
    }

    return FMath::Max(0.0f, NextArtilleryAvailableTime - GetWorld()->GetTimeSeconds());
}

void AWPBattlefieldDirector::IssueSquadOrder(EWPSquadOrder NewOrder, const FVector& OrderLocation)
{
    if (bBattleWon || bBattleLost)
    {
        return;
    }

    CurrentSquadOrder = NewOrder;

    if (NewOrder == EWPSquadOrder::AttackObjective)
    {
        SquadOrderLocation = GetActiveObjectiveLocation();
    }
    else
    {
        SquadOrderLocation = OrderLocation;
    }
}

void AWPBattlefieldDirector::ReportEnemyContact(
    EWPCombatTeam ReportingTeam,
    AWPSoldierCharacter* Contact)
{
    if (!Contact || !Contact->IsCombatCapable() || !GetWorld())
    {
        return;
    }

    if (ReportingTeam == EWPCombatTeam::Allies)
    {
        AlliedSharedContactLocation = Contact->GetActorLocation();
        AlliedSharedContactTime = GetWorld()->GetTimeSeconds();
    }
    else
    {
        AxisSharedContactLocation = Contact->GetActorLocation();
        AxisSharedContactTime = GetWorld()->GetTimeSeconds();
    }
}

bool AWPBattlefieldDirector::GetRecentEnemyContactLocation(
    EWPCombatTeam ReportingTeam,
    FVector& OutLocation,
    float MaximumAgeSeconds) const
{
    if (!GetWorld())
    {
        return false;
    }

    const float Time = GetWorld()->GetTimeSeconds();
    const float ContactTime =
        ReportingTeam == EWPCombatTeam::Allies
            ? AlliedSharedContactTime
            : AxisSharedContactTime;

    if (Time - ContactTime > FMath::Max(0.0f, MaximumAgeSeconds))
    {
        return false;
    }

    OutLocation =
        ReportingTeam == EWPCombatTeam::Allies
            ? AlliedSharedContactLocation
            : AxisSharedContactLocation;

    return true;
}

void AWPBattlefieldDirector::ReportGunshot(
    EWPCombatTeam ShooterTeam,
    const FVector& Location,
    float AudibleRadiusCm)
{
    if (!GetWorld())
    {
        return;
    }

    const float Time = GetWorld()->GetTimeSeconds();
    const float Radius = FMath::Max(0.0f, AudibleRadiusCm);

    if (ShooterTeam == EWPCombatTeam::Allies)
    {
        AlliedGunshotLocation = Location;
        AlliedGunshotTime = Time;
        AlliedGunshotRadiusCm = Radius;
    }
    else
    {
        AxisGunshotLocation = Location;
        AxisGunshotTime = Time;
        AxisGunshotRadiusCm = Radius;
    }
}

bool AWPBattlefieldDirector::GetRecentEnemyGunshot(
    EWPCombatTeam ListeningTeam,
    const FVector& ListenerLocation,
    FVector& OutLocation,
    float MaximumAgeSeconds) const
{
    if (!GetWorld())
    {
        return false;
    }

    const bool bListeningAllies = ListeningTeam == EWPCombatTeam::Allies;
    const FVector& SoundLocation =
        bListeningAllies ? AxisGunshotLocation : AlliedGunshotLocation;
    const float SoundTime =
        bListeningAllies ? AxisGunshotTime : AlliedGunshotTime;
    const float AudibleRadius =
        bListeningAllies ? AxisGunshotRadiusCm : AlliedGunshotRadiusCm;

    const float Age = GetWorld()->GetTimeSeconds() - SoundTime;
    if (Age > FMath::Max(0.0f, MaximumAgeSeconds) ||
        FVector::DistSquared2D(ListenerLocation, SoundLocation) > FMath::Square(AudibleRadius))
    {
        return false;
    }

    OutLocation = SoundLocation;
    return true;
}

bool AWPBattlefieldDirector::RequestArtillery(
    AWPSoldierCharacter* Caller,
    const FVector& TargetLocation)
{
    if (!Caller ||
        !GetWorld() ||
        !ArtilleryStrikeClass ||
        Caller->GetCombatTeam() != EWPCombatTeam::Allies ||
        !Caller->IsCombatCapable() ||
        ArtilleryCharges <= 0 ||
        GetArtilleryCooldownRemaining() > 0.0f ||
        bBattleWon ||
        bBattleLost)
    {
        return false;
    }

    if (FVector::DistSquared2D(Caller->GetActorLocation(), TargetLocation) > FMath::Square(9000.0f))
    {
        return false;
    }

    if (AWPArtilleryStrike* Strike = GetWorld()->SpawnActor<AWPArtilleryStrike>(
        ArtilleryStrikeClass,
        TargetLocation,
        FRotator::ZeroRotator))
    {
        Strike->InitializeStrike(TargetLocation, EWPCombatTeam::Allies);
        --ArtilleryCharges;
        NextArtilleryAvailableTime = GetWorld()->GetTimeSeconds() + ArtilleryCooldownSeconds;
        return true;
    }

    return false;
}

void AWPBattlefieldDirector::SpawnBox(
    const FVector& Location,
    const FVector& Scale,
    const FRotator& Rotation)
{
    if (!GetWorld() || !CubeMesh)
    {
        return;
    }

    AStaticMeshActor* Box = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    if (!Box)
    {
        return;
    }

    UStaticMeshComponent* MeshComponent = Box->GetStaticMeshComponent();
    MeshComponent->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetStaticMesh(CubeMesh);
    MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
    MeshComponent->SetCastShadow(true);
    Box->SetActorScale3D(Scale);

    const bool bThinHorizontalCover =
        Scale.Z > 0.65f &&
        (Scale.X <= 0.65f || Scale.Y <= 0.65f);

    if (bThinHorizontalCover)
    {
        Box->Tags.AddUnique(TEXT("WP_Penetrable"));
    }
}

void AWPBattlefieldDirector::SpawnDestructibleCover(
    const FVector& Location,
    const FVector& Scale,
    float Health)
{
    if (!GetWorld())
    {
        return;
    }

    if (AWPDestructibleCover* Cover = GetWorld()->SpawnActor<AWPDestructibleCover>(
        AWPDestructibleCover::StaticClass(),
        Location,
        FRotator::ZeroRotator))
    {
        Cover->ConfigureCover(Scale, Health);
    }
}

void AWPBattlefieldDirector::BuildBattlefield()
{
    if (!GetWorld())
    {
        return;
    }

    SpawnBox(FVector(0.0f, 0.0f, -30.0f), FVector(60.0f, 60.0f, 0.6f));

    SpawnBox(FVector(-1750.0f, -500.0f, 280.0f), FVector(7.0f, 5.0f, 3.4f));
    SpawnBox(FVector(1650.0f, -1450.0f, 320.0f), FVector(8.0f, 6.0f, 3.8f));
    SpawnBox(FVector(-1900.0f, -2550.0f, 260.0f), FVector(6.5f, 5.0f, 3.2f));
    SpawnBox(FVector(1850.0f, -3300.0f, 300.0f), FVector(7.5f, 5.5f, 3.6f));

    SpawnBox(FVector(-600.0f, 850.0f, 60.0f), FVector(4.0f, 0.5f, 0.8f), FRotator(0.0f, 8.0f, 0.0f));
    SpawnBox(FVector(650.0f, -1100.0f, 60.0f), FVector(3.5f, 0.5f, 0.8f), FRotator(0.0f, -12.0f, 0.0f));
    SpawnBox(FVector(-850.0f, -2850.0f, 55.0f), FVector(5.0f, 0.45f, 0.7f));
    SpawnBox(FVector(900.0f, -3900.0f, 55.0f), FVector(4.5f, 0.45f, 0.7f));

    SpawnDestructibleCover(FVector(-300.0f, 450.0f, 70.0f), FVector(1.4f, 0.55f, 0.75f), 150.0f);
    SpawnDestructibleCover(FVector(320.0f, 300.0f, 70.0f), FVector(1.2f, 0.65f, 0.75f), 150.0f);
    SpawnDestructibleCover(FVector(-350.0f, -1650.0f, 80.0f), FVector(1.6f, 0.7f, 0.85f), 190.0f);
    SpawnDestructibleCover(FVector(450.0f, -2050.0f, 80.0f), FVector(1.5f, 0.65f, 0.85f), 190.0f);
    SpawnDestructibleCover(FVector(-500.0f, -3300.0f, 80.0f), FVector(1.7f, 0.7f, 0.85f), 210.0f);
    SpawnDestructibleCover(FVector(450.0f, -3650.0f, 80.0f), FVector(1.6f, 0.7f, 0.85f), 210.0f);

    SpawnBox(FVector(-5900.0f, 0.0f, 250.0f), FVector(1.0f, 60.0f, 3.0f));
    SpawnBox(FVector(5900.0f, 0.0f, 250.0f), FVector(1.0f, 60.0f, 3.0f));
    SpawnBox(FVector(0.0f, -5900.0f, 250.0f), FVector(60.0f, 1.0f, 3.0f));
    SpawnBox(FVector(0.0f, 5900.0f, 250.0f), FVector(60.0f, 1.0f, 3.0f));

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector::ZeroVector,
        FRotator(-38.0f, -28.0f, 0.0f));
    if (Sun && Sun->GetLightComponent())
    {
        Sun->GetLightComponent()->SetIntensity(7.5f);
        Sun->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.86f, 0.69f));
        Sun->GetLightComponent()->SetCastShadows(true);
    }

    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();
    if (Sky && Sky->GetLightComponent())
    {
        Sky->GetLightComponent()->SetIntensity(0.75f);
        Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sky->GetLightComponent()->RecaptureSky();
    }

    AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>();
    if (Fog && Fog->GetComponent())
    {
        Fog->GetComponent()->SetFogDensity(0.012f);
        Fog->GetComponent()->SetFogHeightFalloff(0.22f);
        Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(0.46f, 0.48f, 0.49f));
        Fog->GetComponent()->SetVolumetricFog(true);
    }
}

void AWPBattlefieldDirector::SpawnCoverPoint(
    const FVector& Location,
    const FVector& CoverNormal,
    float Quality)
{
    if (!GetWorld() || !CoverPointClass)
    {
        return;
    }

    if (AWPCoverPoint* CoverPoint = GetWorld()->SpawnActor<AWPCoverPoint>(
        CoverPointClass,
        Location,
        CoverNormal.Rotation()))
    {
        CoverPoint->InitializeCover(CoverNormal, Quality);
        CoverPoints.Add(CoverPoint);
    }
}

void AWPBattlefieldDirector::BuildCoverPoints()
{
    SpawnCoverPoint(FVector(-1000.0f, 1050.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.2f);
    SpawnCoverPoint(FVector(-200.0f, 1050.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.1f);
    SpawnCoverPoint(FVector(650.0f, 1050.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.1f);

    SpawnCoverPoint(FVector(-1050.0f, -950.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.2f);
    SpawnCoverPoint(FVector(250.0f, -950.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.0f);
    SpawnCoverPoint(FVector(1050.0f, -950.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.15f);

    SpawnCoverPoint(FVector(-1400.0f, -2750.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.35f);
    SpawnCoverPoint(FVector(-450.0f, -2750.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.15f);
    SpawnCoverPoint(FVector(550.0f, -2750.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.15f);
    SpawnCoverPoint(FVector(1450.0f, -2750.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.35f);

    SpawnCoverPoint(FVector(-1200.0f, -3900.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.1f);
    SpawnCoverPoint(FVector(0.0f, -3900.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.0f);
    SpawnCoverPoint(FVector(1200.0f, -3900.0f, 95.0f), FVector(0.0f, 1.0f, 0.0f), 1.1f);
}

void AWPBattlefieldDirector::BuildObjectives()
{
    if (!GetWorld() || !ObjectiveClass)
    {
        return;
    }

    static const FVector ObjectiveLocations[] =
    {
        FVector(0.0f, 1050.0f, 40.0f),
        FVector(100.0f, -1250.0f, 40.0f),
        FVector(-150.0f, -3550.0f, 40.0f)
    };

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ObjectiveLocations); ++Index)
    {
        if (AWPCaptureObjective* Objective = GetWorld()->SpawnActor<AWPCaptureObjective>(
            ObjectiveClass,
            ObjectiveLocations[Index],
            FRotator::ZeroRotator))
        {
            Objective->InitializeObjective(Index);
            Objective->SetObjectiveActive(Index == 0);
            Objective->OnCaptured.AddDynamic(this, &AWPBattlefieldDirector::HandleObjectiveCaptured);
            Objectives.Add(Objective);
        }
    }
}

AWPSoldierCharacter* AWPBattlefieldDirector::SpawnFriendlySoldier(
    const FVector& Location,
    EWPSoldierRole Role,
    APlayerController* ControllerToPossess)
{
    if (!GetWorld() || !AISoldierClass)
    {
        return nullptr;
    }

    const FTransform SpawnTransform(FRotator(0.0f, -90.0f, 0.0f), Location);

    AWPEnemySoldier* Soldier = GetWorld()->SpawnActorDeferred<AWPEnemySoldier>(
        AISoldierClass,
        SpawnTransform,
        nullptr,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

    if (!Soldier)
    {
        return nullptr;
    }

    Soldier->SetCombatTeam(EWPCombatTeam::Allies);
    Soldier->SetSoldierRole(Role);
    Soldier->FinishSpawning(SpawnTransform);

    FriendlySquad.Add(Soldier);
    ++FriendlySoldiersAlive;

    if (ControllerToPossess)
    {
        ControllerToPossess->Possess(Soldier);
        ControllerToPossess->SetControlRotation(Soldier->GetActorRotation());
    }

    return Soldier;
}

void AWPBattlefieldDirector::SpawnFriendlySquad()
{
    static const FVector Offsets[] =
    {
        FVector(-260.0f, 80.0f, 0.0f),
        FVector(260.0f, 80.0f, 0.0f),
        FVector(-420.0f, 280.0f, 0.0f),
        FVector(420.0f, 280.0f, 0.0f),
        FVector(0.0f, 430.0f, 0.0f)
    };

    static const EWPSoldierRole Roles[] =
    {
        EWPSoldierRole::Rifleman,
        EWPSoldierRole::Assault,
        EWPSoldierRole::Medic,
        EWPSoldierRole::Engineer,
        EWPSoldierRole::Rifleman
    };

    const FVector BaseSpawn(0.0f, 2850.0f, 120.0f);

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Offsets); ++Index)
    {
        SpawnFriendlySoldier(BaseSpawn + Offsets[Index], Roles[Index]);
    }
}

bool AWPBattlefieldDirector::IsFriendlySpawnLocationSafe(const FVector& Location) const
{
    if (!GetWorld())
    {
        return false;
    }

    const FVector SpawnEye = Location + FVector(0.0f, 0.0f, 55.0f);

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        const AWPSoldierCharacter* Soldier = *It;
        if (!Soldier ||
            Soldier->GetCombatTeam() != EWPCombatTeam::Axis ||
            !Soldier->IsCombatCapable())
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared2D(
            Location,
            Soldier->GetActorLocation());

        if (DistanceSquared < FMath::Square(1450.0f))
        {
            return false;
        }

        if (DistanceSquared < FMath::Square(3600.0f))
        {
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(WPSpawnSafety), true);
            Params.AddIgnoredActor(Soldier);

            const FVector EnemyEye =
                Soldier->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);

            const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
                Hit,
                SpawnEye,
                EnemyEye,
                ECC_Visibility,
                Params);

            if (!bBlocked)
            {
                return false;
            }
        }
    }

    return true;
}

FVector AWPBattlefieldDirector::GetFriendlySpawnLocation() const
{
    static const FVector SpawnAnchors[] =
    {
        FVector(0.0f, 2850.0f, 120.0f),
        FVector(0.0f, 1650.0f, 120.0f),
        FVector(0.0f, -450.0f, 120.0f),
        FVector(0.0f, -2600.0f, 120.0f)
    };

    const int32 SpawnIndex = FMath::Clamp(
        ActiveObjectiveIndex,
        0,
        UE_ARRAY_COUNT(SpawnAnchors) - 1);

    const FVector Anchor = SpawnAnchors[SpawnIndex];
    FVector BestCandidate = Anchor;
    float BestNearestEnemyDistanceSquared = -1.0f;

    for (int32 Attempt = 0; Attempt < 12; ++Attempt)
    {
        const float Angle = FMath::FRandRange(0.0f, UE_TWO_PI);
        const float Radius = FMath::FRandRange(120.0f, 620.0f);
        const FVector Candidate = Anchor + FVector(
            FMath::Cos(Angle) * Radius,
            FMath::Sin(Angle) * Radius,
            0.0f);

        if (IsFriendlySpawnLocationSafe(Candidate))
        {
            return Candidate;
        }

        float NearestEnemyDistanceSquared = TNumericLimits<float>::Max();
        for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
        {
            const AWPSoldierCharacter* Soldier = *It;
            if (!Soldier ||
                Soldier->GetCombatTeam() != EWPCombatTeam::Axis ||
                !Soldier->IsCombatCapable())
            {
                continue;
            }

            NearestEnemyDistanceSquared = FMath::Min(
                NearestEnemyDistanceSquared,
                FVector::DistSquared2D(Candidate, Soldier->GetActorLocation()));
        }

        if (NearestEnemyDistanceSquared > BestNearestEnemyDistanceSquared)
        {
            BestNearestEnemyDistanceSquared = NearestEnemyDistanceSquared;
            BestCandidate = Candidate;
        }
    }

    return BestCandidate;
}

void AWPBattlefieldDirector::QueueFriendlyReplacement(APlayerController* ControllerNeedingPawn)
{
    if (AlliedTickets <= 0 || bBattleWon || bBattleLost || !GetWorld())
    {
        if (ControllerNeedingPawn && FriendlySoldiersAlive <= 0)
        {
            bBattleLost = true;
        }
        return;
    }

    ++PendingFriendlyReplacements;

    if (ControllerNeedingPawn)
    {
        PendingPlayerRespawnController = ControllerNeedingPawn;
    }

    if (!GetWorld()->GetTimerManager().IsTimerActive(FriendlyReplacementTimer))
    {
        GetWorld()->GetTimerManager().SetTimer(
            FriendlyReplacementTimer,
            this,
            &AWPBattlefieldDirector::SpawnQueuedFriendlyReplacement,
            FriendlyReplacementDelay,
            false);
    }
}

void AWPBattlefieldDirector::SpawnQueuedFriendlyReplacement()
{
    if (PendingFriendlyReplacements <= 0 || AlliedTickets <= 0 || bBattleWon || bBattleLost)
    {
        if (FriendlySoldiersAlive <= 0 && AlliedTickets <= 0)
        {
            bBattleLost = true;
        }
        return;
    }

    static const EWPSoldierRole ReplacementRoles[] =
    {
        EWPSoldierRole::Rifleman,
        EWPSoldierRole::Assault,
        EWPSoldierRole::Medic,
        EWPSoldierRole::Engineer
    };

    const EWPSoldierRole Role = ReplacementRoles[
        ReplacementRoleCursor % UE_ARRAY_COUNT(ReplacementRoles)];
    ++ReplacementRoleCursor;

    APlayerController* RespawnController = PendingPlayerRespawnController.Get();
    AWPSoldierCharacter* Replacement = SpawnFriendlySoldier(
        GetFriendlySpawnLocation(),
        Role,
        RespawnController);

    if (Replacement)
    {
        --AlliedTickets;
        --PendingFriendlyReplacements;

        if (RespawnController)
        {
            PendingPlayerRespawnController.Reset();
        }
    }

    if (PendingFriendlyReplacements > 0 && AlliedTickets > 0 && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            FriendlyReplacementTimer,
            this,
            &AWPBattlefieldDirector::SpawnQueuedFriendlyReplacement,
            FriendlyReplacementDelay,
            false);
    }
}

void AWPBattlefieldDirector::SpawnWave()
{
    if (AxisTickets <= 0)
    {
        return;
    }

    SpawnEnemyGroup(FMath::Min(6 + Wave * 2, 20), true);
}

void AWPBattlefieldDirector::SpawnEnemyGroup(int32 Count, bool bResetEnemyCounter)
{
    if (!GetWorld() || !AISoldierClass || Count <= 0 || AxisTickets <= 0)
    {
        return;
    }

    static const FVector SpawnPoints[] =
    {
        FVector(-2200.0f, -4200.0f, 120.0f),
        FVector(-1200.0f, -4700.0f, 120.0f),
        FVector(0.0f, -5000.0f, 120.0f),
        FVector(1200.0f, -4700.0f, 120.0f),
        FVector(2200.0f, -4200.0f, 120.0f),
        FVector(-3100.0f, -3000.0f, 120.0f),
        FVector(3100.0f, -3000.0f, 120.0f),
        FVector(-3600.0f, -900.0f, 120.0f),
        FVector(3600.0f, -900.0f, 120.0f)
    };

    static const EWPSoldierRole EnemyRoles[] =
    {
        EWPSoldierRole::Rifleman,
        EWPSoldierRole::Rifleman,
        EWPSoldierRole::Assault,
        EWPSoldierRole::Medic,
        EWPSoldierRole::Engineer
    };

    if (bResetEnemyCounter)
    {
        RemainingEnemies = 0;
    }

    const int32 ActualCount = FMath::Min(Count, AxisTickets);

    for (int32 Index = 0; Index < ActualCount; ++Index)
    {
        const FVector Base = SpawnPoints[Index % UE_ARRAY_COUNT(SpawnPoints)];
        const FVector Jitter(
            FMath::FRandRange(-240.0f, 240.0f),
            FMath::FRandRange(-240.0f, 240.0f),
            0.0f);

        const FTransform SpawnTransform(
            FRotator(0.0f, 90.0f, 0.0f),
            Base + Jitter);

        if (AWPEnemySoldier* Enemy = GetWorld()->SpawnActorDeferred<AWPEnemySoldier>(
            AISoldierClass,
            SpawnTransform,
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
        {
            Enemy->SetCombatTeam(EWPCombatTeam::Axis);
            Enemy->SetSoldierRole(EnemyRoles[Index % UE_ARRAY_COUNT(EnemyRoles)]);
            Enemy->FinishSpawning(SpawnTransform);
            ++RemainingEnemies;
            --AxisTickets;
        }
    }
}

void AWPBattlefieldDirector::NotifySoldierKilled(AWPSoldierCharacter* Soldier)
{
    if (!Soldier)
    {
        return;
    }

    if (Soldier->GetCombatTeam() == EWPCombatTeam::Allies)
    {
        const bool bWasPlayerControlled = Soldier->IsPlayerControlled();
        APlayerController* FallenController = bWasPlayerControlled
            ? Cast<APlayerController>(Soldier->GetController())
            : nullptr;

        FriendlySquad.Remove(Soldier);
        FriendlySoldiersAlive = FMath::Max(0, FriendlySoldiersAlive - 1);

        bool bTransferred = false;
        if (bWasPlayerControlled)
        {
            bTransferred = TransferPlayerControl(Soldier);
        }

        QueueFriendlyReplacement(
            bWasPlayerControlled && !bTransferred
                ? FallenController
                : nullptr);

        if (FriendlySoldiersAlive <= 0 && AlliedTickets <= 0)
        {
            bBattleLost = true;
        }

        return;
    }

    RemainingEnemies = FMath::Max(0, RemainingEnemies - 1);
    Score += 100;

    if (RemainingEnemies > 0 || !GetWorld())
    {
        return;
    }

    if (bFinalCounterattack)
    {
        if (ActiveEnemyVehicles <= 0)
        {
            bBattleWon = true;
        }
        return;
    }

    if (AxisTickets <= 0)
    {
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        NextWaveTimer,
        this,
        &AWPBattlefieldDirector::SpawnNextWave,
        InterWaveDelay,
        false);
}

void AWPBattlefieldDirector::HandleObjectiveCaptured(AWPCaptureObjective* Objective)
{
    if (!Objective ||
        !Objectives.IsValidIndex(ActiveObjectiveIndex) ||
        Objectives[ActiveObjectiveIndex] != Objective ||
        !GetWorld())
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(NextWaveTimer);
    Score += 750;

    if (ActiveObjectiveIndex + 1 < Objectives.Num())
    {
        ++ActiveObjectiveIndex;
        Objectives[ActiveObjectiveIndex]->SetObjectiveActive(true);
        SquadOrderLocation = GetActiveObjectiveLocation();
        ++Wave;

        if (ActiveObjectiveIndex >= 2 && !bAlliedArmorSpawned)
        {
            bAlliedArmorSpawned = SpawnArmoredVehicle(
                EWPCombatTeam::Allies,
                FVector(2350.0f, -450.0f, 76.0f),
                FRotator(0.0f, -115.0f, 0.0f)) != nullptr;
        }

        if (AxisTickets > 0)
        {
            SpawnEnemyGroup(4 + ActiveObjectiveIndex * 2, false);
        }
        return;
    }

    bFinalCounterattack = true;
    ++Wave;

    if (AxisTickets > 0)
    {
        SpawnEnemyGroup(12, false);
    }

    if (!bAxisArmorSpawned)
    {
        bAxisArmorSpawned = SpawnArmoredVehicle(
            EWPCombatTeam::Axis,
            FVector(-2350.0f, -4850.0f, 76.0f),
            FRotator(0.0f, 65.0f, 0.0f)) != nullptr;
    }

    if (RemainingEnemies == 0 && ActiveEnemyVehicles == 0)
    {
        bBattleWon = true;
    }
}

AWPArmoredVehicle* AWPBattlefieldDirector::SpawnArmoredVehicle(
    EWPCombatTeam Team,
    const FVector& Location,
    const FRotator& Rotation)
{
    if (!GetWorld() || !ArmoredVehicleClass)
    {
        return nullptr;
    }

    const FTransform SpawnTransform(Rotation, Location);
    AWPArmoredVehicle* Vehicle = GetWorld()->SpawnActorDeferred<AWPArmoredVehicle>(
        ArmoredVehicleClass,
        SpawnTransform,
        nullptr,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

    if (!Vehicle)
    {
        return nullptr;
    }

    Vehicle->SetCombatTeam(Team);
    Vehicle->FinishSpawning(SpawnTransform);

    if (Team == EWPCombatTeam::Axis)
    {
        ++ActiveEnemyVehicles;
    }

    return Vehicle;
}

void AWPBattlefieldDirector::NotifyVehicleDestroyed(EWPCombatTeam VehicleTeam)
{
    if (VehicleTeam == EWPCombatTeam::Axis)
    {
        ActiveEnemyVehicles = FMath::Max(0, ActiveEnemyVehicles - 1);
        Score += 900;

        if (bFinalCounterattack &&
            RemainingEnemies == 0 &&
            ActiveEnemyVehicles == 0)
        {
            bBattleWon = true;
        }
    }
}

bool AWPBattlefieldDirector::TransferPlayerControl(AWPSoldierCharacter* FallenSoldier)
{
    if (!FallenSoldier)
    {
        return false;
    }

    APlayerController* PlayerController = Cast<APlayerController>(FallenSoldier->GetController());
    if (!PlayerController)
    {
        return false;
    }

    AWPSoldierCharacter* BestCandidate = nullptr;
    float BestDistanceSquared = TNumericLimits<float>::Max();

    for (AWPSoldierCharacter* Candidate : FriendlySquad)
    {
        if (!Candidate ||
            Candidate == FallenSoldier ||
            !Candidate->IsCombatCapable())
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared(
            FallenSoldier->GetActorLocation(),
            Candidate->GetActorLocation());

        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestCandidate = Candidate;
        }
    }

    if (!BestCandidate)
    {
        return false;
    }

    PlayerController->Possess(BestCandidate);
    PlayerController->SetControlRotation(BestCandidate->GetActorRotation());
    return true;
}

void AWPBattlefieldDirector::SpawnNextWave()
{
    if (bBattleWon || bBattleLost || bFinalCounterattack || AxisTickets <= 0)
    {
        return;
    }

    ++Wave;
    SpawnWave();
}
