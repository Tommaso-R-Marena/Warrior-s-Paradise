#include "Vehicles/WPArmoredVehicle.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPBallisticProjectile.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSmokeCloud.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Game/WPBattlefieldDirector.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/WPTankShell.h"

AWPArmoredVehicle::AWPArmoredVehicle()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionRoot = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionRoot"));
    SetRootComponent(CollisionRoot);
    CollisionRoot->InitBoxExtent(FVector(230.0f, 115.0f, 75.0f));
    CollisionRoot->SetCollisionProfileName(TEXT("Pawn"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    HullVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullVisual"));
    HullVisual->SetupAttachment(CollisionRoot);
    HullVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HullVisual->SetRelativeScale3D(FVector(4.6f, 2.2f, 1.25f));
    if (CubeAsset.Succeeded())
    {
        HullVisual->SetStaticMesh(CubeAsset.Object);
    }

    TurretVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretVisual"));
    TurretVisual->SetupAttachment(CollisionRoot);
    TurretVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TurretVisual->SetRelativeLocation(FVector(-15.0f, 0.0f, 115.0f));
    TurretVisual->SetRelativeScale3D(FVector(1.7f, 1.5f, 0.7f));
    if (CubeAsset.Succeeded())
    {
        TurretVisual->SetStaticMesh(CubeAsset.Object);
    }

    BarrelVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelVisual"));
    BarrelVisual->SetupAttachment(CollisionRoot);
    BarrelVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BarrelVisual->SetRelativeLocation(FVector(170.0f, 0.0f, 120.0f));
    BarrelVisual->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    BarrelVisual->SetRelativeScale3D(FVector(0.09f, 0.09f, 2.2f));
    if (CylinderAsset.Succeeded())
    {
        BarrelVisual->SetStaticMesh(CylinderAsset.Object);
    }

    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->UpdatedComponent = CollisionRoot;
    Movement->MaxSpeed = 500.0f;
    Movement->Acceleration = 650.0f;
    Movement->Deceleration = 850.0f;
    Movement->TurningBoost = 4.0f;

    TankShellClass = AWPTankShell::StaticClass();
}

UPawnMovementComponent* AWPArmoredVehicle::GetMovementComponent() const
{
    return Movement;
}

void AWPArmoredVehicle::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    BattlefieldDirector = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass()));
    OnTakeAnyDamage.AddDynamic(this, &AWPArmoredVehicle::HandleTakeAnyDamage);
}

AActor* AWPArmoredVehicle::FindBestTarget() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    AActor* BestTarget = nullptr;
    float BestScore = TNumericLimits<float>::Max();

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Soldier = *It;
        if (!Soldier ||
            Soldier->GetCombatTeam() == CombatTeam ||
            !Soldier->IsCombatCapable())
        {
            continue;
        }

        if (!HasLineOfSightTo(Soldier))
        {
            continue;
        }

        float Score = FVector::DistSquared2D(
            GetActorLocation(),
            Soldier->GetActorLocation()) * 0.72f;

        if (Score < BestScore)
        {
            BestScore = Score;
            BestTarget = Soldier;
        }
    }

    for (TActorIterator<AWPArmoredVehicle> It(GetWorld()); It; ++It)
    {
        AWPArmoredVehicle* Vehicle = *It;
        if (!Vehicle ||
            Vehicle == this ||
            Vehicle->GetCombatTeam() == CombatTeam ||
            Vehicle->IsDestroyed())
        {
            continue;
        }

        if (!HasLineOfSightTo(Vehicle))
        {
            continue;
        }

        float Score = FVector::DistSquared2D(
            GetActorLocation(),
            Vehicle->GetActorLocation()) * 0.36f;

        if (Score < BestScore)
        {
            BestScore = Score;
            BestTarget = Vehicle;
        }
    }

    return BestTarget;
}

bool AWPArmoredVehicle::HasLineOfSightTo(const AActor* Target) const
{
    if (!Target || !GetWorld())
    {
        return false;
    }

    const FVector Start = BarrelVisual->GetComponentLocation();
    const FVector End = GetTargetAimPoint(Target);

    for (TActorIterator<AWPSmokeCloud> It(GetWorld()); It; ++It)
    {
        const AWPSmokeCloud* Smoke = *It;
        if (Smoke && Smoke->BlocksSightBetween(Start, End))
        {
            return false;
        }
    }

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WPVehicleSight), true, this);
    Params.AddIgnoredActor(this);

    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        return true;
    }

    return Hit.GetActor() == Target;
}

FVector AWPArmoredVehicle::GetTargetAimPoint(const AActor* Target) const
{
    if (const AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(Target))
    {
        return Soldier->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
    }

    return Target ? Target->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f) : FVector::ZeroVector;
}

void AWPArmoredVehicle::FireCannon(AActor* Target)
{
    if (!Target || !GetWorld() || !TankShellClass)
    {
        return;
    }

    const FVector MuzzleLocation = BarrelVisual->GetComponentLocation() + BarrelVisual->GetForwardVector() * 120.0f;
    const FVector AimDirection = (GetTargetAimPoint(Target) - MuzzleLocation).GetSafeNormal();

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = this;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AWPTankShell* Shell = GetWorld()->SpawnActor<AWPTankShell>(
        TankShellClass,
        MuzzleLocation,
        AimDirection.Rotation(),
        SpawnParameters))
    {
        Shell->InitializeShell(CombatTeam, AimDirection * 22000.0f);
        OnCannonFired.Broadcast();
    }
}

void AWPArmoredVehicle::FireMachineGun(AActor* Target)
{
    if (!Target || !GetWorld())
    {
        return;
    }

    const FVector MuzzleLocation = TurretVisual->GetComponentLocation() + TurretVisual->GetForwardVector() * 120.0f;
    FVector AimDirection = (GetTargetAimPoint(Target) - MuzzleLocation).GetSafeNormal();
    AimDirection = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(1.0f));

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = this;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AWPBallisticProjectile* Projectile = GetWorld()->SpawnActor<AWPBallisticProjectile>(
        AWPBallisticProjectile::StaticClass(),
        MuzzleLocation,
        AimDirection.Rotation(),
        SpawnParameters))
    {
        Projectile->InitializeProjectile(
            AimDirection,
            85000.0f,
            18.0f,
            45000.0f,
            nullptr,
            CombatTeam);

        OnMachineGunFired.Broadcast();
    }
}

void AWPArmoredVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bDestroyed || !GetWorld())
    {
        return;
    }

    const float Time = GetWorld()->GetTimeSeconds();

    if (!CurrentTarget.IsValid() && Time >= NextTargetScanTime)
    {
        CurrentTarget = FindBestTarget();
        NextTargetScanTime = Time + 0.45f;
    }

    AActor* Target = CurrentTarget.Get();
    if (!Target)
    {
        if (!BattlefieldDirector.IsValid())
        {
            BattlefieldDirector = Cast<AWPBattlefieldDirector>(
                UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass()));
        }

        if (AWPBattlefieldDirector* Director = BattlefieldDirector.Get())
        {
            const FVector Destination = Director->GetActiveObjectiveLocation();
            FVector ToDestination = Destination - GetActorLocation();
            ToDestination.Z = 0.0f;

            if (ToDestination.SizeSquared() > FMath::Square(650.0f))
            {
                AddMovementInput(ToDestination.GetSafeNormal(), 0.55f, true);
            }
        }

        return;
    }

    if (const AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(Target))
    {
        if (!Soldier->IsCombatCapable())
        {
            CurrentTarget.Reset();
            return;
        }
    }

    if (const AWPArmoredVehicle* Vehicle = Cast<AWPArmoredVehicle>(Target))
    {
        if (Vehicle->IsDestroyed())
        {
            CurrentTarget.Reset();
            return;
        }
    }

    const FVector TargetLocation = GetTargetAimPoint(Target);
    FVector ToTarget = TargetLocation - GetActorLocation();
    const float Distance = ToTarget.Size();
    FVector FlatDirection(ToTarget.X, ToTarget.Y, 0.0f);
    FlatDirection.Normalize();

    if (!FlatDirection.IsNearlyZero())
    {
        const FRotator DesiredHullRotation = FlatDirection.Rotation();
        SetActorRotation(FMath::RInterpTo(
            GetActorRotation(),
            DesiredHullRotation,
            DeltaSeconds,
            1.2f));

        const FRotator DesiredTurretRotation = (TargetLocation - TurretVisual->GetComponentLocation()).Rotation();
        TurretVisual->SetWorldRotation(FMath::RInterpTo(
            TurretVisual->GetComponentRotation(),
            FRotator(0.0f, DesiredTurretRotation.Yaw, 0.0f),
            DeltaSeconds,
            2.7f));

        BarrelVisual->SetWorldRotation(FMath::RInterpTo(
            BarrelVisual->GetComponentRotation(),
            DesiredTurretRotation,
            DeltaSeconds,
            2.7f));
    }

    if (Distance > DesiredVehicleRangeCm)
    {
        AddMovementInput(FlatDirection, 0.72f, true);
    }

    const bool bVisible = HasLineOfSightTo(Target);

    if (!bVisible)
    {
        CurrentTarget.Reset();
        return;
    }

    if (Distance <= CannonRangeCm && Time >= NextCannonTime)
    {
        FireCannon(Target);
        NextCannonTime = Time + CannonCooldownSeconds + FMath::FRandRange(0.0f, 1.0f);
    }

    if (Distance <= MachineGunRangeCm && Time >= NextMachineGunTime)
    {
        FireMachineGun(Target);
        NextMachineGunTime = Time + MachineGunIntervalSeconds;
    }

}

void AWPArmoredVehicle::ApplyShellImpact(
    float BaseDamage,
    const FVector& ImpactLocation,
    const FVector& ShotDirection)
{
    if (bDestroyed || BaseDamage <= 0.0f)
    {
        return;
    }

    const FVector IncomingFrom = (-ShotDirection).GetSafeNormal2D();
    const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
    const float FacingDot = FVector::DotProduct(Forward, IncomingFrom);

    float ArmorDamageScale = 0.86f;
    if (FacingDot >= 0.55f)
    {
        ArmorDamageScale = 0.55f;
    }
    else if (FacingDot <= -0.55f)
    {
        ArmorDamageScale = 1.35f;
    }

    const float HeightAboveHull = ImpactLocation.Z - GetActorLocation().Z;
    if (HeightAboveHull > 95.0f)
    {
        ArmorDamageScale *= 0.90f;
    }

    const float EffectiveDamage = BaseDamage * ArmorDamageScale;
    Health = FMath::Max(0.0f, Health - EffectiveDamage);
    OnVehicleDamaged.Broadcast(EffectiveDamage, GetHealthNormalized());

    if (Health <= 0.0f)
    {
        DestroyVehicle();
    }
}

void AWPArmoredVehicle::HandleTakeAnyDamage(
    AActor* DamagedActor,
    float Damage,
    const UDamageType* DamageType,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
    if (bDestroyed || Damage <= 0.0f)
    {
        return;
    }

    // Rifle-caliber small arms cannot meaningfully penetrate armored hulls.
    // Explosives, artillery and tank shells cross the threshold and apply normally.
    const float EffectiveDamage = Damage < 60.0f ? 0.0f : Damage;
    Health = FMath::Max(0.0f, Health - EffectiveDamage);

    if (EffectiveDamage > 0.0f)
    {
        OnVehicleDamaged.Broadcast(EffectiveDamage, GetHealthNormalized());
    }

    if (Health <= 0.0f)
    {
        DestroyVehicle();
    }
}

void AWPArmoredVehicle::DestroyVehicle()
{
    if (bDestroyed)
    {
        return;
    }

    bDestroyed = true;
    OnVehicleDestroyed.Broadcast();
    Movement->StopMovementImmediately();
    CollisionRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (AWPBattlefieldDirector* Director = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass())))
    {
        Director->NotifyVehicleDestroyed(CombatTeam);
    }

    HullVisual->SetVisibility(false, true);
    TurretVisual->SetVisibility(false, true);
    BarrelVisual->SetVisibility(false, true);
    SetLifeSpan(1.5f);
}
