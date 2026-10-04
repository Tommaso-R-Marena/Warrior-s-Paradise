#include "Game/WPBattlefieldDirector.h"
#include "AI/WPCoverPoint.h"
#include "AI/WPEnemySoldier.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/TextureCube.h"
#include "EngineUtils.h"
#include "Game/WPArtilleryStrike.h"
#include "Game/WPCaptureObjective.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Presentation/WPAirSupportVisualProfile.h"
#include "Presentation/WPBattlePresentationSet.h"
#include "Presentation/WPEmplacementVisualProfile.h"
#include "Presentation/WPEnvironmentVisualProfile.h"
#include "Presentation/WPExplosionPresentationProfile.h"
#include "Presentation/WPInfantryLoadoutProfile.h"
#include "Presentation/WPLightVehicleVisualProfile.h"
#include "Presentation/WPSmokePresentationProfile.h"
#include "Presentation/WPVehicleVisualProfile.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/WPAirSupportAircraft.h"
#include "Vehicles/WPArmoredVehicle.h"
#include "Vehicles/WPLightVehicle.h"
#include "World/WPDestructibleCover.h"
#include "World/WPGunEmplacement.h"

AWPBattlefieldDirector::AWPBattlefieldDirector()
{
    PrimaryActorTick.bCanEverTick = false;

    AISoldierClass = AWPEnemySoldier::StaticClass();
    ObjectiveClass = AWPCaptureObjective::StaticClass();
    CoverPointClass = AWPCoverPoint::StaticClass();
    ArtilleryStrikeClass = AWPArtilleryStrike::StaticClass();
    ArmoredVehicleClass = AWPArmoredVehicle::StaticClass();
    LightVehicleClass = AWPLightVehicle::StaticClass();
    GunEmplacementClass = AWPGunEmplacement::StaticClass();
    AirSupportClass = AWPAirSupportAircraft::StaticClass();

    BattlePresentationSetAsset = TSoftObjectPtr<UWPBattlePresentationSet>(
        FSoftObjectPath(TEXT(
            "/Game/WarriorParadise/Profiles/"
            "DA_WP_BattlePresentation_Free.DA_WP_BattlePresentation_Free")));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeAsset.Succeeded())
    {
        CubeMesh = CubeAsset.Object;
    }
}

void AWPBattlefieldDirector::BeginPlay()
{
    Super::BeginPlay();

    BattlePresentationSet = BattlePresentationSetAsset.LoadSynchronous();
    EnvironmentVisualProfile = BattlePresentationSet
        ? BattlePresentationSet->Environment
        : nullptr;

    BuildBattlefield();
    ScatterEnvironmentProps();
    BuildCoverPoints();

    AWPSoldierCharacter* Player = Cast<AWPSoldierCharacter>(
        UGameplayStatics::GetPlayerCharacter(this, 0));

    if (Player)
    {
        Player->SetCombatTeam(EWPCombatTeam::Allies);
        Player->SetSoldierRole(EWPSoldierRole::Rifleman);

        if (BattlePresentationSet)
        {
            Player->SetLoadoutProfile(
                BattlePresentationSet->GetInfantryLoadout(
                    EWPCombatTeam::Allies,
                    EWPSoldierRole::Rifleman));
        }

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

        FriendlySquad.AddUnique(Player);
        FriendlySoldiersAlive = 1;
    }

    BuildObjectives();
    SquadOrderLocation = GetActiveObjectiveLocation();
    SpawnFriendlySquad();
    SpawnInitialCombinedArms();
    SpawnDefensiveEmplacements();
    SpawnWave();
    ScheduleAmbientFlyby();
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

bool AWPBattlefieldDirector::IsActiveObjectiveContested() const
{
    return Objectives.IsValidIndex(ActiveObjectiveIndex) &&
        Objectives[ActiveObjectiveIndex] &&
        Objectives[ActiveObjectiveIndex]->IsContested();
}

int32 AWPBattlefieldDirector::GetActiveObjectiveAlliedPresence() const
{
    return Objectives.IsValidIndex(ActiveObjectiveIndex) &&
        Objectives[ActiveObjectiveIndex]
        ? Objectives[ActiveObjectiveIndex]->GetAlliedPresence()
        : 0;
}

int32 AWPBattlefieldDirector::GetActiveObjectiveAxisPresence() const
{
    return Objectives.IsValidIndex(ActiveObjectiveIndex) &&
        Objectives[ActiveObjectiveIndex]
        ? Objectives[ActiveObjectiveIndex]->GetAxisPresence()
        : 0;
}

UWPExplosionPresentationProfile*
AWPBattlefieldDirector::GetFragExplosionPresentation() const
{
    return BattlePresentationSet
        ? BattlePresentationSet->GetFragExplosion()
        : nullptr;
}

UWPExplosionPresentationProfile*
AWPBattlefieldDirector::GetArtilleryExplosionPresentation() const
{
    return BattlePresentationSet
        ? BattlePresentationSet->GetArtilleryExplosion()
        : nullptr;
}

UWPSmokePresentationProfile*
AWPBattlefieldDirector::GetSmokePresentation() const
{
    return BattlePresentationSet
        ? BattlePresentationSet->GetSmokePresentation()
        : nullptr;
}

float AWPBattlefieldDirector::GetArtilleryCooldownRemaining() const
{
    if (!GetWorld())
    {
        return 0.0f;
    }

    return FMath::Max(0.0f, NextArtilleryAvailableTime - GetWorld()->GetTimeSeconds());
}

float AWPBattlefieldDirector::GetAirSupportCooldownRemaining() const
{
    if (!GetWorld())
    {
        return 0.0f;
    }

    return FMath::Max(
        0.0f,
        NextAirSupportAvailableTime -
            GetWorld()->GetTimeSeconds());
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

bool AWPBattlefieldDirector::RequestAirSupport(
    AWPSoldierCharacter* Caller,
    const FVector& TargetLocation)
{
    if (!Caller ||
        Caller->GetCombatTeam() != EWPCombatTeam::Allies ||
        !Caller->IsCombatCapable() ||
        !GetWorld() ||
        bBattleWon ||
        bBattleLost ||
        AirSupportCharges <= 0)
    {
        return false;
    }

    const float Time = GetWorld()->GetTimeSeconds();
    if (Time < NextAirSupportAvailableTime)
    {
        return false;
    }

    bool bArmorNearTarget = false;

    for (TActorIterator<AWPArmoredVehicle> It(GetWorld()); It; ++It)
    {
        const AWPArmoredVehicle* Vehicle = *It;
        if (Vehicle &&
            Vehicle->GetCombatTeam() == EWPCombatTeam::Axis &&
            !Vehicle->IsDestroyed() &&
            FVector::DistSquared2D(
                TargetLocation,
                Vehicle->GetActorLocation()) <=
                FMath::Square(1250.0f))
        {
            bArmorNearTarget = true;
            break;
        }
    }

    if (!bArmorNearTarget)
    {
        for (TActorIterator<AWPLightVehicle> It(GetWorld()); It; ++It)
        {
            const AWPLightVehicle* Vehicle = *It;
            if (Vehicle &&
                Vehicle->GetCombatTeam() == EWPCombatTeam::Axis &&
                !Vehicle->IsDestroyed() &&
                FVector::DistSquared2D(
                    TargetLocation,
                    Vehicle->GetActorLocation()) <=
                    FMath::Square(1250.0f))
            {
                bArmorNearTarget = true;
                break;
            }
        }
    }

    const EWPAirSupportMission Mission =
        bArmorNearTarget
            ? EWPAirSupportMission::Bombing
            : EWPAirSupportMission::Strafe;

    if (!SpawnAirSupport(
        EWPCombatTeam::Allies,
        TargetLocation,
        Mission))
    {
        return false;
    }

    --AirSupportCharges;
    NextAirSupportAvailableTime =
        Time + AirSupportCooldownSeconds;
    return true;
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

bool AWPBattlefieldDirector::SwitchPlayerToSquadmate(
    AWPSoldierCharacter* CurrentSoldier)
{
    if (!CurrentSoldier || bBattleWon || bBattleLost)
    {
        return false;
    }

    APlayerController* PlayerController =
        Cast<APlayerController>(CurrentSoldier->GetController());

    if (!PlayerController)
    {
        return false;
    }

    AWPSoldierCharacter* NextHigher = nullptr;
    AWPSoldierCharacter* Lowest = nullptr;
    uint32 NextHigherId = MAX_uint32;
    uint32 LowestId = MAX_uint32;
    const uint32 CurrentId = CurrentSoldier->GetUniqueID();

    for (AWPSoldierCharacter* Candidate : FriendlySquad)
    {
        if (!Candidate ||
            Candidate == CurrentSoldier ||
            !Candidate->IsCombatCapable())
        {
            continue;
        }

        const uint32 CandidateId = Candidate->GetUniqueID();

        if (CandidateId < LowestId)
        {
            LowestId = CandidateId;
            Lowest = Candidate;
        }

        if (CandidateId > CurrentId && CandidateId < NextHigherId)
        {
            NextHigherId = CandidateId;
            NextHigher = Candidate;
        }
    }

    AWPSoldierCharacter* Destination = NextHigher ? NextHigher : Lowest;
    if (!Destination)
    {
        return false;
    }

    CurrentSoldier->GetWeaponComponent()->StopFire();
    CurrentSoldier->GetWeaponComponent()->SetAiming(false);

    Destination->GetWeaponComponent()->StopFire();
    Destination->GetWeaponComponent()->SetAiming(false);

    PlayerController->Possess(Destination);
    PlayerController->SetControlRotation(Destination->GetActorRotation());
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
        Strike->SetPresentationProfile(
            GetArtilleryExplosionPresentation());
        Strike->InitializeStrike(TargetLocation, EWPCombatTeam::Allies);
        --ArtilleryCharges;
        NextArtilleryAvailableTime = GetWorld()->GetTimeSeconds() + ArtilleryCooldownSeconds;
        return true;
    }

    return false;
}

UMaterialInterface* AWPBattlefieldDirector::ResolveSurfaceMaterial(
    EWPEnvironmentSurface Surface) const
{
    if (!EnvironmentVisualProfile)
    {
        return nullptr;
    }

    return EnvironmentVisualProfile
        ->GetMaterial(Surface)
        .LoadSynchronous();
}

bool AWPBattlefieldDirector::SpawnBuildingVisual(
    const FVector& Location,
    const FVector& BoxScale,
    const FRotator& Rotation)
{
    if (!GetWorld() || !EnvironmentVisualProfile)
    {
        return false;
    }

    UStaticMesh* BuildingMesh =
        EnvironmentVisualProfile->VillageBuildingMesh.LoadSynchronous();

    if (!BuildingMesh)
    {
        return false;
    }

    const FBoxSphereBounds Bounds = BuildingMesh->GetBounds();
    const FVector SourceExtent = Bounds.BoxExtent;

    if (SourceExtent.X <= KINDA_SMALL_NUMBER ||
        SourceExtent.Y <= KINDA_SMALL_NUMBER ||
        SourceExtent.Z <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const FVector DesiredExtent(
        FMath::Abs(BoxScale.X) * 50.0f,
        FMath::Abs(BoxScale.Y) * 50.0f,
        FMath::Abs(BoxScale.Z) * 50.0f);

    FVector VisualScale(
        DesiredExtent.X / SourceExtent.X,
        DesiredExtent.Y / SourceExtent.Y,
        DesiredExtent.Z / SourceExtent.Z);

    VisualScale.X = FMath::Clamp(VisualScale.X, 0.05f, 20.0f);
    VisualScale.Y = FMath::Clamp(VisualScale.Y, 0.05f, 20.0f);
    VisualScale.Z = FMath::Clamp(VisualScale.Z, 0.05f, 20.0f);

    const FVector Offset =
        Rotation.RotateVector(
            EnvironmentVisualProfile->VillageBuildingLocationOffset);

    const FRotator VisualRotation =
        Rotation + EnvironmentVisualProfile->VillageBuildingRotationOffset;

    AStaticMeshActor* Visual = GetWorld()->SpawnActor<AStaticMeshActor>(
        Location + Offset,
        VisualRotation);

    if (!Visual)
    {
        return false;
    }

    UStaticMeshComponent* Component = Visual->GetStaticMeshComponent();
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetStaticMesh(BuildingMesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCastShadow(true);
    Visual->SetActorScale3D(VisualScale);
    return true;
}

void AWPBattlefieldDirector::SpawnEnterableBuilding(
    const FVector& Location,
    const FVector& Scale,
    const FRotator& Rotation,
    EWPEnvironmentSurface WallSurface)
{
    const bool bHasBuildingVisual =
        SpawnBuildingVisual(Location, Scale, Rotation);

    constexpr float WallThicknessScale = 0.25f;
    constexpr float RoofThicknessScale = 0.18f;

    const float HalfWidthCm = FMath::Abs(Scale.X) * 50.0f;
    const float HalfDepthCm = FMath::Abs(Scale.Y) * 50.0f;
    const float HalfHeightCm = FMath::Abs(Scale.Z) * 50.0f;
    const float WallHalfThicknessCm = WallThicknessScale * 50.0f;

    auto ToWorld = [&Location, &Rotation](const FVector& LocalOffset)
    {
        return Location + Rotation.RotateVector(LocalOffset);
    };

    SpawnBox(
        ToWorld(FVector(-HalfWidthCm + WallHalfThicknessCm, 0.0f, 0.0f)),
        FVector(WallThicknessScale, Scale.Y, Scale.Z),
        Rotation,
        WallSurface,
        bHasBuildingVisual);

    SpawnBox(
        ToWorld(FVector(HalfWidthCm - WallHalfThicknessCm, 0.0f, 0.0f)),
        FVector(WallThicknessScale, Scale.Y, Scale.Z),
        Rotation,
        WallSurface,
        bHasBuildingVisual);

    SpawnBox(
        ToWorld(FVector(0.0f, -HalfDepthCm + WallHalfThicknessCm, 0.0f)),
        FVector(Scale.X, WallThicknessScale, Scale.Z),
        Rotation,
        WallSurface,
        bHasBuildingVisual);

    const float DoorScaleX = FMath::Clamp(
        FMath::Min(1.4f, Scale.X * 0.35f),
        0.9f,
        Scale.X * 0.5f);

    const float FrontSegmentScaleX =
        FMath::Max(0.1f, (Scale.X - DoorScaleX) * 0.5f);

    const float FrontSegmentOffsetCm =
        50.0f * (DoorScaleX + FrontSegmentScaleX);

    const float FrontY =
        HalfDepthCm - WallHalfThicknessCm;

    SpawnBox(
        ToWorld(FVector(-FrontSegmentOffsetCm, FrontY, 0.0f)),
        FVector(FrontSegmentScaleX, WallThicknessScale, Scale.Z),
        Rotation,
        WallSurface,
        bHasBuildingVisual);

    SpawnBox(
        ToWorld(FVector(FrontSegmentOffsetCm, FrontY, 0.0f)),
        FVector(FrontSegmentScaleX, WallThicknessScale, Scale.Z),
        Rotation,
        WallSurface,
        bHasBuildingVisual);

    const float FullHeightCm = FMath::Abs(Scale.Z) * 100.0f;
    const float DoorHeightCm = FMath::Min(220.0f, FullHeightCm * 0.75f);
    const float HeaderHeightCm =
        FMath::Max(0.0f, FullHeightCm - DoorHeightCm);

    if (HeaderHeightCm > 10.0f)
    {
        SpawnBox(
            ToWorld(FVector(
                0.0f,
                FrontY,
                DoorHeightCm * 0.5f)),
            FVector(
                DoorScaleX,
                WallThicknessScale,
                HeaderHeightCm / 100.0f),
            Rotation,
            WallSurface,
            bHasBuildingVisual);
    }

    SpawnBox(
        ToWorld(FVector(
            0.0f,
            0.0f,
            HalfHeightCm + RoofThicknessScale * 50.0f)),
        FVector(
            Scale.X * 1.04f,
            Scale.Y * 1.04f,
            RoofThicknessScale),
        Rotation,
        EWPEnvironmentSurface::Roof,
        bHasBuildingVisual);
}

void AWPBattlefieldDirector::SpawnBox(
    const FVector& Location,
    const FVector& Scale,
    const FRotator& Rotation,
    EWPEnvironmentSurface Surface,
    bool bCollisionOnly)
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

    if (UMaterialInterface* Material = ResolveSurfaceMaterial(Surface))
    {
        MeshComponent->SetMaterial(0, Material);
    }

    Box->SetActorScale3D(Scale);

    switch (Surface)
    {
        case EWPEnvironmentSurface::Wood:
        case EWPEnvironmentSurface::Roof:
            Box->Tags.AddUnique(TEXT("WP_Surface_Wood"));
            break;

        case EWPEnvironmentSurface::Metal:
            Box->Tags.AddUnique(TEXT("WP_Surface_Metal"));
            break;

        case EWPEnvironmentSurface::Brick:
            Box->Tags.AddUnique(TEXT("WP_Surface_Brick"));
            break;

        case EWPEnvironmentSurface::Plaster:
            Box->Tags.AddUnique(TEXT("WP_Surface_Plaster"));
            break;

        case EWPEnvironmentSurface::Concrete:
            Box->Tags.AddUnique(TEXT("WP_Surface_Concrete"));
            break;

        case EWPEnvironmentSurface::Ground:
            Box->Tags.AddUnique(TEXT("WP_Surface_Dirt"));
            break;

        case EWPEnvironmentSurface::Stone:
        default:
            Box->Tags.AddUnique(TEXT("WP_Surface_Stone"));
            break;
    }

    if (bCollisionOnly)
    {
        MeshComponent->SetVisibility(false, true);
    }

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
    float Health,
    EWPEnvironmentSurface Surface)
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

        switch (Surface)
        {
            case EWPEnvironmentSurface::Wood:
            case EWPEnvironmentSurface::Roof:
                Cover->Tags.AddUnique(TEXT("WP_Surface_Wood"));
                break;

            case EWPEnvironmentSurface::Metal:
                Cover->Tags.AddUnique(TEXT("WP_Surface_Metal"));
                break;

            case EWPEnvironmentSurface::Brick:
                Cover->Tags.AddUnique(TEXT("WP_Surface_Brick"));
                break;

            case EWPEnvironmentSurface::Plaster:
                Cover->Tags.AddUnique(TEXT("WP_Surface_Plaster"));
                break;

            case EWPEnvironmentSurface::Concrete:
                Cover->Tags.AddUnique(TEXT("WP_Surface_Concrete"));
                break;

            case EWPEnvironmentSurface::Ground:
                Cover->Tags.AddUnique(TEXT("WP_Surface_Dirt"));
                break;

            case EWPEnvironmentSurface::Stone:
            default:
                Cover->Tags.AddUnique(TEXT("WP_Surface_Stone"));
                break;
        }

        if (UMaterialInterface* Material = ResolveSurfaceMaterial(Surface))
        {
            Cover->ApplyMaterial(Material);
        }
    }
}

void AWPBattlefieldDirector::BuildBattlefield()
{
    if (!GetWorld())
    {
        return;
    }

    SpawnBox(
        FVector(0.0f, 0.0f, -30.0f),
        FVector(60.0f, 60.0f, 0.6f),
        FRotator::ZeroRotator,
        EWPEnvironmentSurface::Ground);

    SpawnEnterableBuilding(
        FVector(-1750.0f, -500.0f, 280.0f),
        FVector(7.0f, 5.0f, 3.4f),
        FRotator(0.0f, 8.0f, 0.0f),
        EWPEnvironmentSurface::Plaster);

    SpawnEnterableBuilding(
        FVector(1650.0f, -1450.0f, 320.0f),
        FVector(8.0f, 6.0f, 3.8f),
        FRotator(0.0f, -12.0f, 0.0f),
        EWPEnvironmentSurface::Brick);

    SpawnEnterableBuilding(
        FVector(-1900.0f, -2550.0f, 260.0f),
        FVector(6.5f, 5.0f, 3.2f),
        FRotator(0.0f, 14.0f, 0.0f),
        EWPEnvironmentSurface::Stone);

    SpawnEnterableBuilding(
        FVector(1850.0f, -3300.0f, 300.0f),
        FVector(7.5f, 5.5f, 3.6f),
        FRotator(0.0f, -6.0f, 0.0f),
        EWPEnvironmentSurface::Plaster);

    SpawnBox(
        FVector(-600.0f, 850.0f, 60.0f),
        FVector(4.0f, 0.5f, 0.8f),
        FRotator(0.0f, 8.0f, 0.0f),
        EWPEnvironmentSurface::Wood);

    SpawnBox(
        FVector(650.0f, -1100.0f, 60.0f),
        FVector(3.5f, 0.5f, 0.8f),
        FRotator(0.0f, -12.0f, 0.0f),
        EWPEnvironmentSurface::Stone);

    SpawnBox(
        FVector(-850.0f, -2850.0f, 55.0f),
        FVector(5.0f, 0.45f, 0.7f),
        FRotator::ZeroRotator,
        EWPEnvironmentSurface::Wood);

    SpawnBox(
        FVector(900.0f, -3900.0f, 55.0f),
        FVector(4.5f, 0.45f, 0.7f),
        FRotator::ZeroRotator,
        EWPEnvironmentSurface::Stone);

    SpawnDestructibleCover(
        FVector(-300.0f, 450.0f, 70.0f),
        FVector(1.4f, 0.55f, 0.75f),
        150.0f,
        EWPEnvironmentSurface::Wood);
    SpawnDestructibleCover(
        FVector(320.0f, 300.0f, 70.0f),
        FVector(1.2f, 0.65f, 0.75f),
        150.0f,
        EWPEnvironmentSurface::Wood);
    SpawnDestructibleCover(
        FVector(-350.0f, -1650.0f, 80.0f),
        FVector(1.6f, 0.7f, 0.85f),
        190.0f,
        EWPEnvironmentSurface::Stone);
    SpawnDestructibleCover(
        FVector(450.0f, -2050.0f, 80.0f),
        FVector(1.5f, 0.65f, 0.85f),
        190.0f,
        EWPEnvironmentSurface::Wood);
    SpawnDestructibleCover(
        FVector(-500.0f, -3300.0f, 80.0f),
        FVector(1.7f, 0.7f, 0.85f),
        210.0f,
        EWPEnvironmentSurface::Stone);
    SpawnDestructibleCover(
        FVector(450.0f, -3650.0f, 80.0f),
        FVector(1.6f, 0.7f, 0.85f),
        210.0f,
        EWPEnvironmentSurface::Wood);

    SpawnBox(
        FVector(-5900.0f, 0.0f, 250.0f),
        FVector(1.0f, 60.0f, 3.0f),
        FRotator::ZeroRotator,
        EWPEnvironmentSurface::Concrete);
    SpawnBox(
        FVector(5900.0f, 0.0f, 250.0f),
        FVector(1.0f, 60.0f, 3.0f),
        FRotator::ZeroRotator,
        EWPEnvironmentSurface::Concrete);
    SpawnBox(
        FVector(0.0f, -5900.0f, 250.0f),
        FVector(60.0f, 1.0f, 3.0f),
        FRotator::ZeroRotator,
        EWPEnvironmentSurface::Concrete);
    SpawnBox(
        FVector(0.0f, 5900.0f, 250.0f),
        FVector(60.0f, 1.0f, 3.0f),
        FRotator::ZeroRotator,
        EWPEnvironmentSurface::Concrete);

    BuildAtmosphere();
    StartEnvironmentAudio();
}

void AWPBattlefieldDirector::BuildAtmosphere()
{
    if (!GetWorld())
    {
        return;
    }

    ASkyAtmosphere* Atmosphere =
        GetWorld()->SpawnActor<ASkyAtmosphere>(
            FVector::ZeroVector,
            FRotator::ZeroRotator);

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector::ZeroVector,
        FRotator(-38.0f, -28.0f, 0.0f));

    if (Sun)
    {
        if (UDirectionalLightComponent* SunComponent =
            Sun->FindComponentByClass<UDirectionalLightComponent>())
        {
            SunComponent->SetIntensity(7.5f);
            SunComponent->SetLightColor(
                FLinearColor(1.0f, 0.86f, 0.69f));
            SunComponent->SetCastShadows(true);
            SunComponent->SetAtmosphereSunLight(true);
            SunComponent->SetAtmosphereSunLightIndex(0);
        }
    }

    UMaterialInterface* CloudMaterial = nullptr;

    if (EnvironmentVisualProfile)
    {
        CloudMaterial =
            EnvironmentVisualProfile->CloudMaterial.LoadSynchronous();
    }

    if (!CloudMaterial)
    {
        CloudMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT(
                "/Engine/EngineSky/VolumetricClouds/"
                "m_SimpleVolumetricCloud_Inst."
                "m_SimpleVolumetricCloud_Inst"));
    }

    if (CloudMaterial)
    {
        if (AVolumetricCloud* Cloud =
            GetWorld()->SpawnActor<AVolumetricCloud>(
                FVector::ZeroVector,
                FRotator::ZeroRotator))
        {
            if (UVolumetricCloudComponent* CloudComponent =
                Cloud->FindComponentByClass<UVolumetricCloudComponent>())
            {
                CloudComponent->SetMaterial(CloudMaterial);
                CloudComponent->SetLayerBottomAltitude(1.5f);
                CloudComponent->SetLayerHeight(5.5f);
                CloudComponent
                    ->SetbUsePerSampleAtmosphericLightTransmittance(true);
                CloudComponent->SetVisibleInRealTimeSkyCaptures(true);
            }
        }
    }

    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();

    if (Sky && Sky->GetLightComponent())
    {
        USkyLightComponent* SkyComponent = Sky->GetLightComponent();
        SkyComponent->SetIntensity(0.75f);
        SkyComponent->SetMobility(EComponentMobility::Movable);

        UTextureCube* EnvironmentCubemap =
            EnvironmentVisualProfile
                ? EnvironmentVisualProfile->SkyCubemap.LoadSynchronous()
                : nullptr;

        if (EnvironmentCubemap)
        {
            SkyComponent->SourceType = SLS_SpecifiedCubemap;
            SkyComponent->SetCubemap(EnvironmentCubemap);
            SkyComponent->SetRealTimeCapture(false);
        }
        else
        {
            SkyComponent->SetRealTimeCapture(true);
            SkyComponent->RecaptureSky();
        }
    }

    AExponentialHeightFog* Fog =
        GetWorld()->SpawnActor<AExponentialHeightFog>();

    if (Fog && Fog->GetComponent())
    {
        Fog->GetComponent()->SetFogDensity(0.012f);
        Fog->GetComponent()->SetFogHeightFalloff(0.22f);
        Fog->GetComponent()->SetFogInscatteringColor(
            FLinearColor::Black);
        Fog->GetComponent()->SetVolumetricFog(true);
    }
}

void AWPBattlefieldDirector::StartEnvironmentAudio()
{
    if (!GetWorld() || !EnvironmentVisualProfile)
    {
        return;
    }

    if (USoundBase* Ambient =
        EnvironmentVisualProfile->AmbientLoop.LoadSynchronous())
    {
        AmbientAudioComponent = UGameplayStatics::SpawnSound2D(
            this,
            Ambient,
            EnvironmentVisualProfile->AmbientVolume,
            1.0f,
            0.0f,
            nullptr,
            false,
            false);
    }

    if (USoundBase* Wind =
        EnvironmentVisualProfile->WindLoop.LoadSynchronous())
    {
        WindAudioComponent = UGameplayStatics::SpawnSound2D(
            this,
            Wind,
            EnvironmentVisualProfile->WindVolume,
            1.0f,
            0.0f,
            nullptr,
            false,
            false);
    }

    if (!EnvironmentVisualProfile->DistantBattleOneShot.IsNull())
    {
        const float MinInterval = FMath::Max(
            1.0f,
            EnvironmentVisualProfile->DistantBattleMinIntervalSeconds);

        const float MaxInterval = FMath::Max(
            MinInterval,
            EnvironmentVisualProfile->DistantBattleMaxIntervalSeconds);

        GetWorld()->GetTimerManager().SetTimer(
            DistantBattleTimer,
            this,
            &AWPBattlefieldDirector::PlayDistantBattleOneShot,
            FMath::FRandRange(MinInterval, MaxInterval),
            false);
    }
}

void AWPBattlefieldDirector::PlayDistantBattleOneShot()
{
    if (!GetWorld() || !EnvironmentVisualProfile)
    {
        return;
    }

    if (USoundBase* Sound =
        EnvironmentVisualProfile->DistantBattleOneShot.LoadSynchronous())
    {
        UGameplayStatics::PlaySound2D(
            this,
            Sound,
            EnvironmentVisualProfile->DistantBattleVolume,
            FMath::FRandRange(0.94f, 1.06f),
            0.0f,
            nullptr,
            this,
            false);
    }

    const float MinInterval = FMath::Max(
        1.0f,
        EnvironmentVisualProfile->DistantBattleMinIntervalSeconds);

    const float MaxInterval = FMath::Max(
        MinInterval,
        EnvironmentVisualProfile->DistantBattleMaxIntervalSeconds);

    GetWorld()->GetTimerManager().SetTimer(
        DistantBattleTimer,
        this,
        &AWPBattlefieldDirector::PlayDistantBattleOneShot,
        FMath::FRandRange(MinInterval, MaxInterval),
        false);
}

void AWPBattlefieldDirector::ScatterEnvironmentProps()
{
    if (!GetWorld() || !EnvironmentVisualProfile)
    {
        return;
    }

    UStaticMesh* TreeMesh = EnvironmentVisualProfile->TreeMesh.LoadSynchronous();
    UStaticMesh* CrateMesh = EnvironmentVisualProfile->CrateMesh.LoadSynchronous();
    UStaticMesh* BarrelMesh = EnvironmentVisualProfile->BarrelMesh.LoadSynchronous();

    auto SpawnDecoration = [this](
        UStaticMesh* Mesh,
        const FVector& Location,
        const FRotator& Rotation,
        const FVector& Scale)
    {
        if (!Mesh)
        {
            return;
        }

        AStaticMeshActor* Decoration =
            GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);

        if (!Decoration)
        {
            return;
        }

        UStaticMeshComponent* Component = Decoration->GetStaticMeshComponent();
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCastShadow(true);
        Decoration->SetActorScale3D(Scale);
    };

    FRandomStream Random(1944);

    if (TreeMesh)
    {
        for (int32 Index = 0; Index < 30; ++Index)
        {
            const bool bLeftRight = (Index % 2) == 0;
            const float X = bLeftRight
                ? Random.FRandRange(-5400.0f, -2800.0f)
                : Random.FRandRange(2800.0f, 5400.0f);
            const float Y = Random.FRandRange(-5000.0f, 5000.0f);
            const float UniformScale = Random.FRandRange(0.82f, 1.35f);

            SpawnDecoration(
                TreeMesh,
                FVector(X, Y, 35.0f),
                FRotator(0.0f, Random.FRandRange(-180.0f, 180.0f), 0.0f),
                FVector(UniformScale));
        }
    }

    static const FVector CrateLocations[] =
    {
        FVector(-480.0f, 1220.0f, 45.0f),
        FVector(560.0f, 900.0f, 45.0f),
        FVector(-520.0f, -1180.0f, 45.0f),
        FVector(680.0f, -1460.0f, 45.0f),
        FVector(-620.0f, -3380.0f, 45.0f),
        FVector(620.0f, -3700.0f, 45.0f)
    };

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(CrateLocations); ++Index)
    {
        SpawnDecoration(
            CrateMesh,
            CrateLocations[Index],
            FRotator(0.0f, static_cast<float>(Index * 37), 0.0f),
            FVector(Random.FRandRange(0.85f, 1.12f)));
    }

    static const FVector BarrelLocations[] =
    {
        FVector(-720.0f, 780.0f, 45.0f),
        FVector(760.0f, 1280.0f, 45.0f),
        FVector(-780.0f, -1540.0f, 45.0f),
        FVector(780.0f, -3050.0f, 45.0f)
    };

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(BarrelLocations); ++Index)
    {
        SpawnDecoration(
            BarrelMesh,
            BarrelLocations[Index],
            FRotator(0.0f, static_cast<float>(Index * 61), 0.0f),
            FVector(Random.FRandRange(0.9f, 1.08f)));
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

    if (BattlePresentationSet)
    {
        Soldier->SetLoadoutProfile(
            BattlePresentationSet->GetInfantryLoadout(
                EWPCombatTeam::Allies,
                Role));
    }

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
            const EWPSoldierRole Role =
                EnemyRoles[Index % UE_ARRAY_COUNT(EnemyRoles)];

            Enemy->SetCombatTeam(EWPCombatTeam::Axis);
            Enemy->SetSoldierRole(Role);

            if (BattlePresentationSet)
            {
                Enemy->SetLoadoutProfile(
                    BattlePresentationSet->GetInfantryLoadout(
                        EWPCombatTeam::Axis,
                        Role));
            }

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

    if (BattlePresentationSet)
    {
        Vehicle->SetVisualProfile(
            BattlePresentationSet->GetVehicleVisual(Team));
    }

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

    BestCandidate->GetWeaponComponent()->StopFire();
    BestCandidate->GetWeaponComponent()->SetAiming(false);

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
