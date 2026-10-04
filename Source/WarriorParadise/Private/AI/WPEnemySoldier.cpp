#include "AI/WPEnemySoldier.h"
#include "AI/WPCoverPoint.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSmokeCloud.h"
#include "Combat/WPSuppressionComponent.h"
#include "Combat/WPWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Game/WPBattlefieldDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AWPEnemySoldier::AWPEnemySoldier()
{
    PrimaryActorTick.bCanEverTick = true;
    SetCombatTeam(EWPCombatTeam::Axis);

    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    GetCharacterMovement()->MaxWalkSpeed = 315.0f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    TorsoVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TorsoVisual"));
    TorsoVisual->SetupAttachment(GetCapsuleComponent());
    TorsoVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TorsoVisual->SetOwnerNoSee(true);
    TorsoVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
    TorsoVisual->SetRelativeScale3D(FVector(0.34f, 0.22f, 0.52f));
    if (CubeAsset.Succeeded())
    {
        TorsoVisual->SetStaticMesh(CubeAsset.Object);
    }

    HeadVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadVisual"));
    HeadVisual->SetupAttachment(GetCapsuleComponent());
    HeadVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeadVisual->SetOwnerNoSee(true);
    HeadVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 76.0f));
    HeadVisual->SetRelativeScale3D(FVector(0.21f));
    if (SphereAsset.Succeeded())
    {
        HeadVisual->SetStaticMesh(SphereAsset.Object);
    }

    RifleVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RifleVisual"));
    RifleVisual->SetupAttachment(GetCapsuleComponent());
    RifleVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RifleVisual->SetOwnerNoSee(true);
    RifleVisual->SetRelativeLocation(FVector(34.0f, 19.0f, 25.0f));
    RifleVisual->SetRelativeScale3D(FVector(0.54f, 0.035f, 0.035f));
    if (CubeAsset.Succeeded())
    {
        RifleVisual->SetStaticMesh(CubeAsset.Object);
    }
}

void AWPEnemySoldier::BeginPlay()
{
    Super::BeginPlay();

    BattlefieldDirector = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass()));

    // Both factions use the same wound lifecycle so medics have a real
    // battlefield role and "eliminated" means final death, not merely 0 HP.
    GetHealthComponent()->SetCanBeDowned(true);

    switch (GetSoldierRole())
    {
        case EWPSoldierRole::Assault:
            DesiredRange = 650.0f;
            MaximumEngagementRange = 2100.0f;
            GetCharacterMovement()->MaxWalkSpeed = 355.0f;
            break;

        case EWPSoldierRole::Medic:
            DesiredRange = 900.0f;
            MaximumEngagementRange = 2350.0f;
            GetCharacterMovement()->MaxWalkSpeed = 340.0f;
            break;

        case EWPSoldierRole::Engineer:
            DesiredRange = 900.0f;
            MaximumEngagementRange = 2450.0f;
            GetCharacterMovement()->MaxWalkSpeed = 325.0f;
            break;

        case EWPSoldierRole::Rifleman:
        default:
            DesiredRange = 1100.0f;
            MaximumEngagementRange = 3000.0f;
            GetCharacterMovement()->MaxWalkSpeed = 330.0f;
            break;
    }

    if (HasProductionVisuals())
    {
        TorsoVisual->SetVisibility(false, true);
        HeadVisual->SetVisibility(false, true);
        RifleVisual->SetVisibility(false, true);
    }

    NextGrenadeTime = GetWorld()
        ? GetWorld()->GetTimeSeconds() + FMath::FRandRange(5.0f, 10.0f)
        : 5.0f;
}

void AWPEnemySoldier::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReleaseCover();
    Super::EndPlay(EndPlayReason);
}

AWPSoldierCharacter* AWPEnemySoldier::FindBestTarget() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    AWPSoldierCharacter* BestTarget = nullptr;
    float BestScore = TNumericLimits<float>::Max();
    const float CloseAwarenessRadiusSquared = FMath::Square(650.0f);

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Candidate = *It;
        if (!Candidate ||
            Candidate == this ||
            Candidate->GetCombatTeam() == GetCombatTeam() ||
            !Candidate->IsCombatCapable())
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared2D(
            GetActorLocation(),
            Candidate->GetActorLocation());

        const bool bVisible = HasLineOfSightTo(Candidate);
        const bool bCloseAwareness = DistanceSquared <= CloseAwarenessRadiusSquared;

        if (!bVisible && !bCloseAwareness)
        {
            continue;
        }

        float Score = DistanceSquared;

        if (GetCombatTeam() == EWPCombatTeam::Axis && Candidate->IsPlayerControlled())
        {
            Score *= 0.88f;
        }

        if (bVisible)
        {
            Score *= 0.68f;
        }

        if (Score < BestScore)
        {
            BestScore = Score;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
}

AWPSoldierCharacter* AWPEnemySoldier::FindDownedAlly(float MaximumRange) const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    AWPSoldierCharacter* BestTarget = nullptr;
    float BestDistanceSquared = FMath::Square(MaximumRange);

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Candidate = *It;
        if (!Candidate ||
            Candidate == this ||
            Candidate->GetCombatTeam() != GetCombatTeam() ||
            !Candidate->IsDowned())
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared2D(
            GetActorLocation(),
            Candidate->GetActorLocation());

        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
}

AWPSoldierCharacter* AWPEnemySoldier::FindBleedingAlly(
    float MaximumRange) const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    AWPSoldierCharacter* BestTarget = nullptr;
    float BestScore =
        FMath::Square(MaximumRange);

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld());
         It;
         ++It)
    {
        AWPSoldierCharacter* Candidate = *It;
        if (!Candidate ||
            Candidate == this ||
            Candidate->GetCombatTeam() != GetCombatTeam() ||
            !Candidate->IsCombatCapable() ||
            !Candidate->GetHealthComponent() ||
            !Candidate->GetHealthComponent()->IsBleeding())
        {
            continue;
        }

        const float DistanceSquared =
            FVector::DistSquared2D(
                GetActorLocation(),
                Candidate->GetActorLocation());

        const float Severity =
            Candidate->GetHealthComponent()
                ->GetBleedingSeverity();

        const float Score =
            DistanceSquared *
            FMath::Lerp(1.0f, 0.52f, Severity);

        if (Score < BestScore)
        {
            BestScore = Score;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
}

AWPSoldierCharacter* AWPEnemySoldier::FindAmmoTarget(float MaximumRange) const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    AWPSoldierCharacter* BestTarget = nullptr;
    float BestDistanceSquared = FMath::Square(MaximumRange);

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Candidate = *It;
        if (!Candidate ||
            Candidate == this ||
            Candidate->GetCombatTeam() != GetCombatTeam() ||
            !Candidate->IsCombatCapable() ||
            !Candidate->GetWeaponComponent() ||
            Candidate->GetWeaponComponent()->GetReserveAmmo() >= 60)
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared2D(
            GetActorLocation(),
            Candidate->GetActorLocation());

        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
}

AWPCoverPoint* AWPEnemySoldier::FindBestCover(const AWPSoldierCharacter* Threat) const
{
    if (!GetWorld() || !Threat)
    {
        return nullptr;
    }

    AWPCoverPoint* BestCover = nullptr;
    float BestScore = TNumericLimits<float>::Max();

    const FVector ThreatEye = Threat->GetActorLocation() + FVector(0.0f, 0.0f, 58.0f);

    for (TActorIterator<AWPCoverPoint> It(GetWorld()); It; ++It)
    {
        AWPCoverPoint* Candidate = *It;
        if (!Candidate || !Candidate->IsAvailableFor(this))
        {
            continue;
        }

        const float OwnDistance = FVector::Dist2D(GetActorLocation(), Candidate->GetActorLocation());
        if (OwnDistance > CoverSearchRadius)
        {
            continue;
        }

        const FVector CoverEye = Candidate->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
        FHitResult Hit;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WPCoverExposure), true, this);
        QueryParams.AddIgnoredActor(this);
        QueryParams.AddIgnoredActor(Threat);

        const bool bOccluded = GetWorld()->LineTraceSingleByChannel(
            Hit,
            ThreatEye,
            CoverEye,
            ECC_Visibility,
            QueryParams);

        const float ThreatDistance = FVector::Dist2D(
            Candidate->GetActorLocation(),
            Threat->GetActorLocation());

        const float RangePenalty = FMath::Abs(ThreatDistance - DesiredRange) * 0.25f;
        const float ExposurePenalty = bOccluded ? -850.0f : 1100.0f;
        const float QualityBonus = -Candidate->GetCoverQuality() * 420.0f;
        const float Score = OwnDistance + RangePenalty + ExposurePenalty + QualityBonus;

        if (Score < BestScore)
        {
            BestScore = Score;
            BestCover = Candidate;
        }
    }

    return BestCover;
}

AWPEnemySoldier* AWPEnemySoldier::SelectManeuverElement(
    const AWPSoldierCharacter* Target,
    int32& OutEligibleShooters) const
{
    OutEligibleShooters = 0;

    if (!Target || !GetWorld())
    {
        return nullptr;
    }

    AWPEnemySoldier* BestCandidate = nullptr;
    float BestScore = TNumericLimits<float>::Max();

    for (TActorIterator<AWPEnemySoldier> It(GetWorld()); It; ++It)
    {
        AWPEnemySoldier* Candidate = *It;
        if (!Candidate ||
            Candidate->IsPlayerControlled() ||
            !Candidate->IsCombatCapable() ||
            Candidate->GetCombatTeam() != GetCombatTeam())
        {
            continue;
        }

        if (FVector::DistSquared2D(
                GetActorLocation(),
                Candidate->GetActorLocation()) >
            FMath::Square(CoordinationRadius))
        {
            continue;
        }

        UWPSuppressionComponent* CandidateSuppression =
            Candidate->GetSuppressionComponent();
        if (CandidateSuppression &&
            CandidateSuppression->IsPinned())
        {
            continue;
        }

        const float TargetDistance =
            FVector::Dist2D(
                Candidate->GetActorLocation(),
                Target->GetActorLocation());

        if (TargetDistance >
                Candidate->MaximumEngagementRange * 1.15f ||
            !Candidate->HasLineOfSightTo(Target))
        {
            continue;
        }

        ++OutEligibleShooters;

        const float Score =
            Candidate->GetManeuverSelectionScore();

        if (Score < BestScore)
        {
            BestScore = Score;
            BestCandidate = Candidate;
        }
    }

    return BestCandidate;
}

float AWPEnemySoldier::GetManeuverSelectionScore() const
{
    float RoleBias = 1000.0f;

    switch (GetSoldierRole())
    {
        case EWPSoldierRole::Assault:
            RoleBias = 0.0f;
            break;

        case EWPSoldierRole::Rifleman:
            RoleBias = 1000.0f;
            break;

        case EWPSoldierRole::Engineer:
            RoleBias = 3000.0f;
            break;

        case EWPSoldierRole::Medic:
            RoleBias = 5000.0f;
            break;

        default:
            break;
    }

    const UWPSuppressionComponent* Suppression =
        GetSuppressionComponent();
    const UWPHealthComponent* Health =
        GetHealthComponent();

    const float SuppressionPenalty =
        Suppression
            ? Suppression->GetNormalizedSuppression() * 4000.0f
            : 0.0f;

    const float BleedingPenalty =
        Health && Health->IsBleeding()
            ? Health->GetBleedingSeverity() * 6000.0f
            : 0.0f;

    const float StableTieBreak =
        static_cast<float>(GetUniqueID() % 997U);

    return
        RoleBias +
        SuppressionPenalty +
        BleedingPenalty +
        StableTieBreak;
}

FVector AWPEnemySoldier::GetManeuverDestination(
    const AWPSoldierCharacter* Target) const
{
    if (!Target)
    {
        return GetActorLocation();
    }

    FVector ToTarget =
        Target->GetActorLocation() - GetActorLocation();
    ToTarget.Z = 0.0f;

    const float Distance = ToTarget.Size();
    const FVector Forward =
        ToTarget.GetSafeNormal();

    if (Forward.IsNearlyZero())
    {
        return GetActorLocation();
    }

    FVector Lateral =
        FVector::CrossProduct(
            FVector::UpVector,
            Forward).GetSafeNormal();

    if ((GetUniqueID() & 1U) != 0U)
    {
        Lateral *= -1.0f;
    }

    const float FlankDistance =
        FMath::Min(
            ManeuverFlankDistance,
            FMath::Max(300.0f, Distance * 0.42f));

    const float Standoff =
        FMath::Max(300.0f, DesiredRange * 0.72f);

    return
        Target->GetActorLocation() -
        Forward * Standoff +
        Lateral * FlankDistance;
}

bool AWPEnemySoldier::HasLineOfSightTo(const AActor* Target) const
{
    if (!Target || !GetWorld())
    {
        return false;
    }

    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 58.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);

    for (TActorIterator<AWPSmokeCloud> It(GetWorld()); It; ++It)
    {
        const AWPSmokeCloud* Smoke = *It;
        if (Smoke && Smoke->BlocksSightBetween(Start, End))
        {
            return false;
        }
    }

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WPEnemySight), true, this);
    Params.AddIgnoredActor(this);

    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        return true;
    }

    return Hit.GetActor() == Target;
}

FVector AWPEnemySoldier::GetFormationOffset() const
{
    const uint32 Slot = GetUniqueID() % 7;
    const float Angle = static_cast<float>(Slot) / 7.0f * UE_TWO_PI;
    const float Radius = 220.0f + static_cast<float>(Slot % 3) * 70.0f;
    return FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
}

FVector AWPEnemySoldier::GetStrategicDestination() const
{
    AWPBattlefieldDirector* Director = BattlefieldDirector.Get();
    if (!Director)
    {
        return GetActorLocation();
    }

    if (GetCombatTeam() == EWPCombatTeam::Axis)
    {
        return Director->GetActiveObjectiveLocation() + GetFormationOffset();
    }

    switch (Director->GetSquadOrder())
    {
        case EWPSquadOrder::Follow:
        {
            if (const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
            {
                return Player->GetActorLocation() + GetFormationOffset();
            }
            break;
        }

        case EWPSquadOrder::HoldPosition:
            return Director->GetSquadOrderLocation() + GetFormationOffset();

        case EWPSquadOrder::AttackObjective:
        default:
            return Director->GetActiveObjectiveLocation() + GetFormationOffset();
    }

    return Director->GetSquadOrderLocation();
}

void AWPEnemySoldier::MoveTacticallyToward(const FVector& Destination, float InputScale)
{
    if (!GetWorld())
    {
        return;
    }

    FVector Direction = Destination - GetActorLocation();
    Direction.Z = 0.0f;

    if (Direction.SizeSquared() < FMath::Square(35.0f))
    {
        return;
    }

    Direction.Normalize();

    const FVector TraceStart = GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
    const FVector TraceEnd = TraceStart + Direction * 190.0f;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WPObstacleSteer), true, this);
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
    {
        FVector Right = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();
        if ((GetUniqueID() & 1U) == 0U)
        {
            Right *= -1.0f;
        }

        Direction = (Direction * 0.22f + Right * 0.95f).GetSafeNormal();
    }

    AddMovementInput(Direction, FMath::Clamp(InputScale, 0.0f, 1.0f));
}

void AWPEnemySoldier::ReleaseCover()
{
    if (AWPCoverPoint* Cover = CurrentCover.Get())
    {
        Cover->Release(this);
    }

    CurrentCover.Reset();
}

bool AWPEnemySoldier::HandleMedicSupport()
{
    if (GetSoldierRole() != EWPSoldierRole::Medic)
    {
        return false;
    }

    AWPSoldierCharacter* MedicalTarget =
        FindDownedAlly(1800.0f);
    const bool bNeedsRevive =
        MedicalTarget != nullptr;

    if (!MedicalTarget &&
        GetMedkitsRemaining() > 0)
    {
        MedicalTarget =
            FindBleedingAlly(1400.0f);
    }

    if (!MedicalTarget)
    {
        return false;
    }

    ReleaseCover();
    GetWeaponComponent()->StopFire();
    bPulseFiring = false;

    const float Distance = FVector::Dist2D(
        GetActorLocation(),
        MedicalTarget->GetActorLocation());
    const float Suppression = GetSuppressionComponent()
        ? GetSuppressionComponent()->GetNormalizedSuppression()
        : 0.0f;

    if (Suppression >= 0.25f &&
        GetSmokeGrenadesRemaining() > 0 &&
        Distance > 300.0f)
    {
        ThrowSmokeGrenadeAt(
            MedicalTarget->GetActorLocation());
    }

    if (Distance <= 210.0f)
    {
        if (bNeedsRevive)
        {
            TryReviveSoldier(MedicalTarget);
        }
        else
        {
            TryTreatSoldier(MedicalTarget);
        }
    }
    else
    {
        MoveTacticallyToward(
            MedicalTarget->GetActorLocation(),
            1.0f);
    }

    return true;
}

bool AWPEnemySoldier::HandleEngineerSupport()
{
    if (GetSoldierRole() != EWPSoldierRole::Engineer || !GetWorld())
    {
        return false;
    }

    const float Time = GetWorld()->GetTimeSeconds();
    if (Time < NextEngineerSupplyTime)
    {
        return false;
    }

    AWPSoldierCharacter* AmmoTarget = FindAmmoTarget(950.0f);
    if (!AmmoTarget)
    {
        return false;
    }

    const float Distance = FVector::Dist2D(GetActorLocation(), AmmoTarget->GetActorLocation());
    if (Distance <= 220.0f)
    {
        AmmoTarget->GetWeaponComponent()->AddReserveAmmo(45);
        NextEngineerSupplyTime = Time + 14.0f;
        return true;
    }

    if (!CurrentTarget.IsValid())
    {
        MoveTacticallyToward(AmmoTarget->GetActorLocation(), 0.85f);
        return true;
    }

    return false;
}

void AWPEnemySoldier::HandleGrenadeOpportunity(AWPSoldierCharacter* Target, float Distance)
{
    if (!Target || !GetWorld())
    {
        return;
    }

    const float Time = GetWorld()->GetTimeSeconds();
    if (Time < NextGrenadeTime ||
        Distance < GrenadeMinimumRange ||
        Distance > GrenadeMaximumRange ||
        !HasLineOfSightTo(Target))
    {
        return;
    }

    const float Suppression = GetSuppressionComponent()
        ? GetSuppressionComponent()->GetNormalizedSuppression()
        : 0.0f;

    if (Suppression > 0.72f)
    {
        return;
    }

    if (FMath::FRand() <= 0.32f &&
        ThrowGrenadeAt(Target->GetActorLocation() + Target->GetVelocity() * 0.35f))
    {
        NextGrenadeTime = Time + GrenadeCooldownSeconds + FMath::FRandRange(0.0f, 6.0f);
    }
}

void AWPEnemySoldier::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (IsPlayerControlled() || !GetWorld() || !IsCombatCapable())
    {
        return;
    }

    if (!BattlefieldDirector.IsValid())
    {
        BattlefieldDirector = Cast<AWPBattlefieldDirector>(
            UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass()));
    }

    const float Time = GetWorld()->GetTimeSeconds();

    UWPSuppressionComponent* SuppressionComponent =
        GetSuppressionComponent();
    const float Suppression = SuppressionComponent
        ? SuppressionComponent->GetNormalizedSuppression()
        : 0.0f;
    const EWPSuppressionState SuppressionState = SuppressionComponent
        ? SuppressionComponent->GetSuppressionState()
        : EWPSuppressionState::Steady;
    const bool bPinned =
        SuppressionState == EWPSuppressionState::Pinned;
    const bool bPressured =
        SuppressionState != EWPSuppressionState::Steady;

    TacticalPosture = bPinned
        ? EWPAITacticalPosture::Pinned
        : EWPAITacticalPosture::Independent;

    if (UWPWeaponComponent* Weapon = GetWeaponComponent())
    {
        const float SuppressionAccuracy =
            bPinned
                ? FMath::Lerp(2.2f, 3.25f, Suppression)
                : FMath::Lerp(1.0f, 2.15f, Suppression);

        Weapon->SetAccuracyMultiplier(
            SuppressionAccuracy);
    }

    if (!bPinned &&
        GetSoldierRole() == EWPSoldierRole::Medic &&
        Time >= NextSupportSearchTime)
    {
        NextSupportSearchTime = Time + 0.35f;
        if (HandleMedicSupport())
        {
            return;
        }
    }

    if (Time >= NextTargetSearchTime ||
        !CurrentTarget.IsValid() ||
        !CurrentTarget->IsCombatCapable())
    {
        CurrentTarget = FindBestTarget();
        NextTargetSearchTime = Time + RetargetInterval + FMath::FRandRange(0.0f, 0.18f);
    }

    AWPSoldierCharacter* Target = CurrentTarget.Get();

    if (!Target)
    {
        GetWeaponComponent()->StopFire();
        bPulseFiring = false;

        if (bPressured)
        {
            if (AWPCoverPoint* Cover = CurrentCover.Get())
            {
                const float CoverDistance = FVector::Dist2D(
                    GetActorLocation(),
                    Cover->GetActorLocation());

                if (CoverDistance > 115.0f)
                {
                    MoveTacticallyToward(
                        Cover->GetActorLocation(),
                        bPinned ? 1.0f : 0.82f);
                }
                return;
            }
        }
        else
        {
            ReleaseCover();
        }

        if (!bPinned && HandleEngineerSupport())
        {
            return;
        }

        if (AWPBattlefieldDirector* Director = BattlefieldDirector.Get())
        {
            FVector InvestigateLocation = FVector::ZeroVector;
            bool bHasInvestigation = Director->GetRecentEnemyContactLocation(
                GetCombatTeam(),
                InvestigateLocation,
                3.5f);

            if (!bHasInvestigation)
            {
                bHasInvestigation = Director->GetRecentEnemyGunshot(
                    GetCombatTeam(),
                    GetActorLocation(),
                    InvestigateLocation,
                    4.5f);
            }

            if (bHasInvestigation)
            {
                if (bPinned)
                {
                    FVector AwayFromThreat =
                        GetActorLocation() - InvestigateLocation;
                    AwayFromThreat.Z = 0.0f;

                    if (!AwayFromThreat.IsNearlyZero())
                    {
                        MoveTacticallyToward(
                            GetActorLocation() +
                                AwayFromThreat.GetSafeNormal() * 600.0f,
                            0.72f);
                    }
                    return;
                }

                const FVector InvestigatePoint =
                    InvestigateLocation + GetFormationOffset() * 0.35f;

                if (FVector::DistSquared2D(GetActorLocation(), InvestigatePoint) >
                    FMath::Square(150.0f))
                {
                    MoveTacticallyToward(
                        InvestigatePoint,
                        bPressured ? 0.62f : 0.88f);
                }
                return;
            }
        }

        const FVector Destination = GetStrategicDestination();
        const float DistanceToDestination = FVector::Dist2D(GetActorLocation(), Destination);
        if (DistanceToDestination > 170.0f)
        {
            MoveTacticallyToward(
                Destination,
                bPinned ? 0.45f : (bPressured ? 0.68f : 0.9f));
        }
        return;
    }

    const bool bTargetVisible = HasLineOfSightTo(Target);
    const float TargetDistance2D = FVector::Dist2D(
        GetActorLocation(),
        Target->GetActorLocation());
    const bool bCloseAwareness = TargetDistance2D <= 650.0f;

    if (bTargetVisible)
    {
        if (AWPBattlefieldDirector* Director = BattlefieldDirector.Get())
        {
            Director->ReportEnemyContact(GetCombatTeam(), Target);
        }
    }
    else if (!bCloseAwareness)
    {
        FVector LastKnownLocation = FVector::ZeroVector;
        if (AWPBattlefieldDirector* Director = BattlefieldDirector.Get())
        {
            if (Director->GetRecentEnemyContactLocation(
                GetCombatTeam(),
                LastKnownLocation,
                3.5f))
            {
                MoveTacticallyToward(
                    LastKnownLocation + GetFormationOffset() * 0.25f,
                    0.82f);
            }
        }

        CurrentTarget.Reset();
        GetWeaponComponent()->StopFire();
        bPulseFiring = false;
        return;
    }

    const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    const float Distance = ToTarget.Size();
    const float HealthNormalized =
        GetHealthComponent()->GetHealthNormalized();
    const FVector FlatDirection =
        FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();

    if (!bPinned &&
        !bPressured &&
        bTargetVisible &&
        Distance > 350.0f)
    {
        int32 EligibleShooters = 0;
        AWPEnemySoldier* ManeuverElement =
            SelectManeuverElement(
                Target,
                EligibleShooters);

        if (EligibleShooters >= 2 &&
            ManeuverElement)
        {
            TacticalPosture =
                ManeuverElement == this &&
                Distance > DesiredRange * 0.68f
                    ? EWPAITacticalPosture::Maneuver
                    : EWPAITacticalPosture::BaseOfFire;

            bHasUsedCoordinatedTactic = true;
        }
    }

    if (Distance <= 180.0f && TryMeleeAttack(Target))
    {
        GetWeaponComponent()->StopFire();
        bPulseFiring = false;
        return;
    }

    if (!FlatDirection.IsNearlyZero())
    {
        const FRotator DesiredRotation = FlatDirection.Rotation();
        SetActorRotation(FMath::RInterpTo(
            GetActorRotation(),
            DesiredRotation,
            DeltaSeconds,
            AimTurnSpeed));
    }

    if (TacticalPosture ==
            EWPAITacticalPosture::Maneuver &&
        CurrentCover.IsValid())
    {
        ReleaseCover();
    }

    const bool bWantsCover =
        bPressured ||
        TacticalPosture ==
            EWPAITacticalPosture::BaseOfFire ||
        Suppression >= CoverTriggerSuppression ||
        HealthNormalized <= 0.45f;

    if (bWantsCover && Time >= NextCoverSearchTime)
    {
        NextCoverSearchTime =
            Time +
            (bPinned ? 0.35f : 1.1f) +
            FMath::FRandRange(0.0f, bPinned ? 0.15f : 0.4f);

        AWPCoverPoint* BetterCover = FindBestCover(Target);
        if (BetterCover && BetterCover != CurrentCover.Get() && BetterCover->TryReserve(this))
        {
            ReleaseCover();
            CurrentCover = BetterCover;
        }
    }

    bool bAtCover = false;
    if (AWPCoverPoint* Cover = CurrentCover.Get())
    {
        const float CoverDistance = FVector::Dist2D(GetActorLocation(), Cover->GetActorLocation());
        bAtCover = CoverDistance <= 115.0f;

        if (!bAtCover)
        {
            MoveTacticallyToward(
                Cover->GetActorLocation(),
                bPinned
                    ? 1.0f
                    : FMath::Lerp(0.95f, 0.7f, Suppression));
        }
    }
    else if (bPinned)
    {
        // If no reservable cover exists, break contact instead of
        // standing in the open trading accurate fire.
        MoveTacticallyToward(
            GetActorLocation() - FlatDirection * 650.0f,
            0.72f);
    }
    else if (TacticalPosture ==
                 EWPAITacticalPosture::Maneuver &&
             Distance > DesiredRange * 0.62f)
    {
        MoveTacticallyToward(
            GetManeuverDestination(Target),
            0.96f);
    }
    else if (TacticalPosture ==
             EWPAITacticalPosture::BaseOfFire)
    {
        if (Distance < DesiredRange * 0.55f)
        {
            MoveTacticallyToward(
                GetActorLocation() -
                    FlatDirection * 450.0f,
                0.38f);
        }
        else
        {
            const FVector Strafe(
                -FlatDirection.Y,
                FlatDirection.X,
                0.0f);

            AddMovementInput(
                Strafe,
                FMath::Sin(
                    Time * 0.55f +
                    static_cast<float>(GetUniqueID())) *
                    0.12f);
        }
    }
    else if (Distance > DesiredRange)
    {
        FVector ApproachPoint = Target->GetActorLocation();

        const uint32 FlankSlot = GetUniqueID() % 5U;
        if (Suppression < 0.35f &&
            (FlankSlot == 0U || FlankSlot == 1U))
        {
            FVector Lateral =
                FVector::CrossProduct(
                    FVector::UpVector,
                    FlatDirection).GetSafeNormal();

            if (FlankSlot == 1U)
            {
                Lateral *= -1.0f;
            }

            ApproachPoint +=
                Lateral *
                FMath::Min(
                    700.0f,
                    Distance * 0.32f);
        }

        MoveTacticallyToward(
            ApproachPoint,
            0.78f *
                FMath::Lerp(
                    1.0f,
                    0.62f,
                    Suppression));
    }
    else if (Distance < DesiredRange * 0.58f)
    {
        MoveTacticallyToward(
            GetActorLocation() - FlatDirection * 500.0f,
            0.38f + Suppression * 0.25f);
    }
    else
    {
        const FVector Strafe(-FlatDirection.Y, FlatDirection.X, 0.0f);
        const float StrafeStrength = FMath::Lerp(0.18f, 0.52f, Suppression);
        AddMovementInput(
            Strafe,
            FMath::Sin(Time * 0.85f + static_cast<float>(GetUniqueID())) * StrafeStrength);
    }

    if (!bPinned)
    {
        HandleGrenadeOpportunity(Target, Distance);
    }

    if (bPinned && !bAtCover && bPulseFiring)
    {
        GetWeaponComponent()->StopFire();
        bPulseFiring = false;
    }

    if (bPulseFiring && Time >= StopFireTime)
    {
        GetWeaponComponent()->StopFire();
        bPulseFiring = false;
    }

    if (Time >= NextDecisionTime)
    {
        NextDecisionTime =
            Time +
            (bPinned
                ? FMath::FRandRange(0.65f, 1.10f)
                : FMath::FRandRange(0.32f, 0.72f));

        const float CoverConfidence =
            bAtCover ? 1.12f : 1.0f;

        float TacticalFireScale = 1.0f;
        if (TacticalPosture ==
            EWPAITacticalPosture::BaseOfFire)
        {
            TacticalFireScale = 1.28f;
        }
        else if (TacticalPosture ==
                 EWPAITacticalPosture::Maneuver)
        {
            TacticalFireScale = 0.32f;
        }

        const float FireChance = bPinned
            ? (bAtCover ? 0.10f : 0.0f)
            : FMath::Clamp(
                FMath::Lerp(
                    1.0f,
                    0.28f,
                    Suppression) *
                    CoverConfidence *
                    TacticalFireScale,
                0.03f,
                1.0f);

        if (Distance <= MaximumEngagementRange &&
            bTargetVisible &&
            FireChance > 0.0f &&
            FMath::FRand() <= FireChance)
        {
            GetWeaponComponent()->StartFire();
            bPulseFiring = true;

            float PulseScale = 1.0f;
            if (TacticalPosture ==
                EWPAITacticalPosture::BaseOfFire)
            {
                PulseScale =
                    BaseOfFirePulseMultiplier;
            }
            else if (TacticalPosture ==
                     EWPAITacticalPosture::Maneuver)
            {
                PulseScale = 0.65f;
            }

            StopFireTime =
                Time +
                FirePulseSeconds *
                    PulseScale *
                    FMath::Lerp(
                        1.0f,
                        0.65f,
                        Suppression);
        }
    }

    if (!bPinned &&
        GetSoldierRole() == EWPSoldierRole::Engineer &&
        Distance > 1300.0f &&
        Time >= NextSupportSearchTime)
    {
        NextSupportSearchTime = Time + 0.6f;
        HandleEngineerSupport();
    }
}
