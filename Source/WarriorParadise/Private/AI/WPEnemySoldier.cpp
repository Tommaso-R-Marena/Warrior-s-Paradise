#include "AI/WPEnemySoldier.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSuppressionComponent.h"
#include "Combat/WPWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
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

    if (GetCombatTeam() == EWPCombatTeam::Allies)
    {
        GetWeaponComponent()->Configure(14.0f, 430.0f, 2.7f);
        GetCharacterMovement()->MaxWalkSpeed = 335.0f;
    }
    else
    {
        GetWeaponComponent()->Configure(9.0f, 360.0f, 4.2f);
        GetCharacterMovement()->MaxWalkSpeed = 315.0f;
    }
}

AWPSoldierCharacter* AWPEnemySoldier::FindBestTarget() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    AWPSoldierCharacter* BestTarget = nullptr;
    float BestScore = TNumericLimits<float>::Max();

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Candidate = *It;
        if (!Candidate || Candidate == this)
        {
            continue;
        }

        if (Candidate->GetCombatTeam() == GetCombatTeam())
        {
            continue;
        }

        if (!Candidate->GetHealthComponent() || Candidate->GetHealthComponent()->IsDead())
        {
            continue;
        }

        float Score = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());

        if (GetCombatTeam() == EWPCombatTeam::Axis && Candidate->IsPlayerControlled())
        {
            Score *= 0.82f;
        }

        if (Score < BestScore)
        {
            BestScore = Score;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
}

bool AWPEnemySoldier::HasLineOfSightTo(const AActor* Target) const
{
    if (!Target || !GetWorld())
    {
        return false;
    }

    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 58.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WPEnemySight), true, this);
    Params.AddIgnoredActor(this);

    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        return true;
    }

    return Hit.GetActor() == Target;
}

void AWPEnemySoldier::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (IsPlayerControlled())
    {
        return;
    }

    if (GetHealthComponent()->IsDead() || !GetWorld())
    {
        return;
    }

    const float Time = GetWorld()->GetTimeSeconds();

    if (Time >= NextTargetSearchTime ||
        !CurrentTarget.IsValid() ||
        !CurrentTarget->GetHealthComponent() ||
        CurrentTarget->GetHealthComponent()->IsDead())
    {
        CurrentTarget = FindBestTarget();
        NextTargetSearchTime = Time + RetargetInterval + FMath::FRandRange(0.0f, 0.18f);
    }

    AWPSoldierCharacter* Target = CurrentTarget.Get();
    if (!Target)
    {
        GetWeaponComponent()->StopFire();
        bPulseFiring = false;
        return;
    }

    const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    const float Distance = ToTarget.Size();
    const float Suppression = GetSuppressionComponent()
        ? GetSuppressionComponent()->GetNormalizedSuppression()
        : 0.0f;
    const FVector FlatDirection = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();

    if (!FlatDirection.IsNearlyZero())
    {
        const FRotator DesiredRotation = FlatDirection.Rotation();
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, AimTurnSpeed));
    }

    if (Distance > DesiredRange)
    {
        AddMovementInput(FlatDirection, 0.75f * FMath::Lerp(1.0f, 0.62f, Suppression));
    }
    else if (Distance < DesiredRange * 0.62f)
    {
        AddMovementInput(-FlatDirection, 0.35f + Suppression * 0.28f);
    }
    else
    {
        const FVector Strafe(-FlatDirection.Y, FlatDirection.X, 0.0f);
        const float StrafeStrength = FMath::Lerp(0.22f, 0.55f, Suppression);
        AddMovementInput(Strafe, FMath::Sin(Time * 0.85f + GetUniqueID()) * StrafeStrength);
    }

    if (bPulseFiring && Time >= StopFireTime)
    {
        GetWeaponComponent()->StopFire();
        bPulseFiring = false;
    }

    if (Time >= NextDecisionTime)
    {
        NextDecisionTime = Time + FMath::FRandRange(0.35f, 0.75f);

        const float FireChance = FMath::Lerp(1.0f, 0.32f, Suppression);
        if (Distance <= MaximumEngagementRange &&
            HasLineOfSightTo(Target) &&
            FMath::FRand() <= FireChance)
        {
            GetWeaponComponent()->StartFire();
            bPulseFiring = true;
            StopFireTime = Time + FirePulseSeconds;
        }
    }
}
