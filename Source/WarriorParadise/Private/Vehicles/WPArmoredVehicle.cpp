#include "Vehicles/WPArmoredVehicle.h"
#include "Characters/WPSoldierCharacter.h"
#include "Camera/CameraComponent.h"
#include "Combat/WPBallisticProjectile.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSmokeCloud.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Game/WPBattlefieldDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/WPVehicleVisualProfile.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/WPTankShell.h"

AWPArmoredVehicle::AWPArmoredVehicle()
{
    PrimaryActorTick.bCanEverTick = true;
    Tags.AddUnique(TEXT("WP_Surface_Metal"));

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

    CrewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CrewCamera"));
    CrewCamera->SetupAttachment(TurretVisual);
    CrewCamera->SetRelativeLocation(FVector(-55.0f, 0.0f, 75.0f));
    CrewCamera->SetRelativeRotation(FRotator(-4.0f, 0.0f, 0.0f));
    CrewCamera->bUsePawnControlRotation = false;

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
    EngineHealth = EngineMaxHealth;
    TrackHealth = TrackMaxHealth;
    TurretHealth = TurretMaxHealth;
    AmmoRackHealth = AmmoRackMaxHealth;
    ApplyVisualProfile();
    BattlefieldDirector = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass()));
    OnTakeAnyDamage.AddDynamic(this, &AWPArmoredVehicle::HandleTakeAnyDamage);
}

void AWPArmoredVehicle::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(
        TEXT("MoveForward"),
        this,
        &AWPArmoredVehicle::MoveForwardInput);
    PlayerInputComponent->BindAxis(
        TEXT("MoveRight"),
        this,
        &AWPArmoredVehicle::TurnHullInput);
    PlayerInputComponent->BindAxis(
        TEXT("Turn"),
        this,
        &AWPArmoredVehicle::TurnTurretInput);
    PlayerInputComponent->BindAxis(
        TEXT("LookUp"),
        this,
        &AWPArmoredVehicle::PitchGunInput);

    PlayerInputComponent->BindAction(
        TEXT("Fire"),
        IE_Pressed,
        this,
        &AWPArmoredVehicle::PlayerFireCannon);
    PlayerInputComponent->BindAction(
        TEXT("Aim"),
        IE_Pressed,
        this,
        &AWPArmoredVehicle::StartPlayerMachineGun);
    PlayerInputComponent->BindAction(
        TEXT("Aim"),
        IE_Released,
        this,
        &AWPArmoredVehicle::StopPlayerMachineGun);
    PlayerInputComponent->BindAction(
        TEXT("Reload"),
        IE_Pressed,
        this,
        &AWPArmoredVehicle::ToggleShellType);
    PlayerInputComponent->BindAction(
        TEXT("Interact"),
        IE_Pressed,
        this,
        &AWPArmoredVehicle::InteractInput);
}

bool AWPArmoredVehicle::TryEnter(AWPSoldierCharacter* Soldier)
{
    if (!Soldier ||
        bDestroyed ||
        CrewSoldier.IsValid() ||
        Soldier->GetCombatTeam() != CombatTeam ||
        !Soldier->IsCombatCapable())
    {
        return false;
    }

    APlayerController* PlayerController =
        Cast<APlayerController>(Soldier->GetController());

    if (!PlayerController)
    {
        return false;
    }

    CrewSoldier = Soldier;
    Soldier->GetWeaponComponent()->StopFire();
    Soldier->SetActorHiddenInGame(true);
    Soldier->SetActorEnableCollision(false);
    Soldier->SetActorTickEnabled(false);
    Soldier->GetCharacterMovement()->DisableMovement();

    PlayerController->Possess(this);
    PlayerController->SetControlRotation(GetActorRotation());

    PlayerTurretYaw = 0.0f;
    PlayerGunPitch = 0.0f;
    bPlayerMachineGunFiring = false;
    return true;
}

void AWPArmoredVehicle::RestoreCrewSoldier(bool bEmergency)
{
    AWPSoldierCharacter* Soldier = CrewSoldier.Get();
    if (!Soldier)
    {
        return;
    }

    Soldier->SetActorHiddenInGame(false);
    Soldier->SetActorEnableCollision(true);
    Soldier->SetActorTickEnabled(true);
    Soldier->SetActorLocation(
        GetActorLocation() +
            GetActorRightVector() * 250.0f +
            FVector(0.0f, 0.0f, 70.0f),
        false,
        nullptr,
        ETeleportType::TeleportPhysics);

    if (!Soldier->GetHealthComponent()->IsDead())
    {
        Soldier->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

        if (bEmergency)
        {
            UGameplayStatics::ApplyDamage(
                Soldier,
                65.0f,
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }
}

void AWPArmoredVehicle::ExitVehicle()
{
    AWPSoldierCharacter* Soldier = CrewSoldier.Get();
    APlayerController* PlayerController =
        Soldier
            ? Cast<APlayerController>(GetController())
            : nullptr;

    if (!Soldier || !PlayerController)
    {
        return;
    }

    PlayerController->Possess(Soldier);
    RestoreCrewSoldier(false);
    CrewSoldier.Reset();
    bPlayerMachineGunFiring = false;
}

void AWPArmoredVehicle::InteractInput()
{
    ExitVehicle();
}

void AWPArmoredVehicle::MoveForwardInput(float Value)
{
    if (bDestroyed ||
        IsImmobilized() ||
        FMath::IsNearlyZero(Value))
    {
        return;
    }

    AddMovementInput(
        GetActorForwardVector(),
        Value,
        true);
}

void AWPArmoredVehicle::TurnHullInput(float Value)
{
    if (bDestroyed ||
        IsImmobilized() ||
        FMath::IsNearlyZero(Value))
    {
        return;
    }

    const float Mobility =
        FMath::Min(
            GetSubsystemHealthNormalized(EWPVehicleSubsystem::Engine),
            GetSubsystemHealthNormalized(EWPVehicleSubsystem::Tracks));

    FRotator Rotation = GetActorRotation();
    Rotation.Yaw += Value * FMath::Lerp(0.35f, 1.15f, Mobility);
    SetActorRotation(Rotation);
}

void AWPArmoredVehicle::TurnTurretInput(float Value)
{
    if (bDestroyed || TurretHealth <= 0.0f)
    {
        return;
    }

    const float TurretFactor =
        GetSubsystemHealthNormalized(EWPVehicleSubsystem::Turret);

    PlayerTurretYaw +=
        Value * FMath::Lerp(0.35f, 1.15f, TurretFactor);

    TurretVisual->SetRelativeRotation(
        FRotator(0.0f, PlayerTurretYaw, 0.0f));

    BarrelVisual->SetRelativeRotation(
        FRotator(PlayerGunPitch, PlayerTurretYaw, 0.0f));
}

void AWPArmoredVehicle::PitchGunInput(float Value)
{
    if (bDestroyed || TurretHealth <= 0.0f)
    {
        return;
    }

    PlayerGunPitch = FMath::Clamp(
        PlayerGunPitch + Value * 0.85f,
        -10.0f,
        22.0f);

    BarrelVisual->SetRelativeRotation(
        FRotator(PlayerGunPitch, PlayerTurretYaw, 0.0f));
}

FVector AWPArmoredVehicle::GetPlayerAimPoint(float RangeCm) const
{
    if (!GetWorld() || !CrewCamera)
    {
        return GetActorLocation() +
            GetActorForwardVector() * RangeCm;
    }

    const FVector Start = CrewCamera->GetComponentLocation();
    const FVector End =
        Start +
        CrewCamera->GetForwardVector() * RangeCm;

    FHitResult Hit;
    FCollisionQueryParams Params(
        SCENE_QUERY_STAT(WPTankPlayerAim),
        true,
        this);
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility,
        Params))
    {
        return Hit.ImpactPoint;
    }

    return End;
}

void AWPArmoredVehicle::ToggleShellType()
{
    SelectedShellType =
        SelectedShellType == EWPArmorShellType::ArmorPiercing
            ? EWPArmorShellType::HighExplosive
            : EWPArmorShellType::ArmorPiercing;
}

void AWPArmoredVehicle::PlayerFireCannon()
{
    if (!GetWorld() ||
        bDestroyed ||
        TurretHealth <= 0.0f)
    {
        return;
    }

    const float Time = GetWorld()->GetTimeSeconds();
    if (Time < NextCannonTime)
    {
        return;
    }

    FireCannonAt(
        GetPlayerAimPoint(CannonRangeCm),
        SelectedShellType);

    const float TurretFactor =
        GetSubsystemHealthNormalized(EWPVehicleSubsystem::Turret);

    NextCannonTime =
        Time +
        CannonCooldownSeconds *
            FMath::Lerp(1.75f, 1.0f, TurretFactor);
}

void AWPArmoredVehicle::StartPlayerMachineGun()
{
    bPlayerMachineGunFiring = true;
}

void AWPArmoredVehicle::StopPlayerMachineGun()
{
    bPlayerMachineGunFiring = false;
}

void AWPArmoredVehicle::ApplyVisualProfile()
{
    bProductionVisualsActive = false;

    if (!VisualProfile)
    {
        return;
    }

    if (UStaticMesh* HullMesh = VisualProfile->HullMesh.LoadSynchronous())
    {
        HullVisual->SetStaticMesh(HullMesh);
        HullVisual->SetRelativeTransform(VisualProfile->HullRelativeTransform);
        bProductionVisualsActive = true;
    }

    if (UStaticMesh* TurretMesh = VisualProfile->TurretMesh.LoadSynchronous())
    {
        TurretVisual->SetStaticMesh(TurretMesh);
        TurretVisual->SetRelativeTransform(VisualProfile->TurretRelativeTransform);
        TurretVisual->SetVisibility(true, true);
        bProductionVisualsActive = true;
    }
    else if (VisualProfile->bHideFallbackTurretWhenMissing)
    {
        TurretVisual->SetVisibility(false, true);
    }

    if (UStaticMesh* BarrelMesh = VisualProfile->BarrelMesh.LoadSynchronous())
    {
        BarrelVisual->SetStaticMesh(BarrelMesh);
        BarrelVisual->SetRelativeTransform(VisualProfile->BarrelRelativeTransform);
        BarrelVisual->SetVisibility(true, true);
        bProductionVisualsActive = true;
    }
    else if (VisualProfile->bHideFallbackBarrelWhenMissing)
    {
        BarrelVisual->SetVisibility(false, true);
    }
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
    if (!Target)
    {
        return;
    }

    const EWPArmorShellType ShellType =
        Cast<AWPArmoredVehicle>(Target)
            ? EWPArmorShellType::ArmorPiercing
            : EWPArmorShellType::HighExplosive;

    FireCannonAt(
        GetTargetAimPoint(Target),
        ShellType);
}

void AWPArmoredVehicle::FireCannonAt(
    const FVector& TargetLocation,
    EWPArmorShellType ShellType)
{
    if (!GetWorld() ||
        !TankShellClass ||
        bDestroyed ||
        TurretHealth <= 0.0f)
    {
        return;
    }

    const FVector MuzzleLocation =
        BarrelVisual->GetComponentLocation() +
        BarrelVisual->GetForwardVector() * 120.0f;

    const FVector AimDirection =
        (TargetLocation - MuzzleLocation).GetSafeNormal();

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = this;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AWPTankShell* Shell =
        GetWorld()->SpawnActor<AWPTankShell>(
            TankShellClass,
            MuzzleLocation,
            AimDirection.Rotation(),
            SpawnParameters))
    {
        Shell->OnImpact.AddUniqueDynamic(
            this,
            &AWPArmoredVehicle::HandleTankShellImpact);

        Shell->InitializeShell(
            CombatTeam,
            AimDirection * 22000.0f,
            ShellType);

        if (VisualProfile)
        {
            if (UNiagaraSystem* System =
                VisualProfile->CannonMuzzleFlash.LoadSynchronous())
            {
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                    this,
                    System,
                    MuzzleLocation,
                    AimDirection.Rotation(),
                    VisualProfile->CannonMuzzleScale,
                    true,
                    true,
                    ENCPoolMethod::AutoRelease,
                    true);
            }

            if (USoundBase* Sound =
                VisualProfile->CannonSound.LoadSynchronous())
            {
                UGameplayStatics::PlaySoundAtLocation(
                    this,
                    Sound,
                    MuzzleLocation,
                    VisualProfile->CannonVolume,
                    1.0f + FMath::FRandRange(-0.015f, 0.015f),
                    0.0f,
                    VisualProfile->Attenuation.LoadSynchronous(),
                    nullptr,
                    nullptr);
            }
        }

        OnCannonFired.Broadcast();
    }
}

void AWPArmoredVehicle::FireMachineGun(AActor* Target)
{
    if (!Target)
    {
        return;
    }

    FireMachineGunAt(GetTargetAimPoint(Target));
}

void AWPArmoredVehicle::FireMachineGunAt(
    const FVector& TargetLocation)
{
    if (!GetWorld() || bDestroyed)
    {
        return;
    }

    const FVector MuzzleLocation =
        TurretVisual->GetComponentLocation() +
        TurretVisual->GetForwardVector() * 120.0f;

    FVector AimDirection =
        (TargetLocation - MuzzleLocation).GetSafeNormal();

    AimDirection = FMath::VRandCone(
        AimDirection,
        FMath::DegreesToRadians(1.0f));

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = this;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AWPBallisticProjectile* Projectile =
        GetWorld()->SpawnActor<AWPBallisticProjectile>(
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

        if (VisualProfile)
        {
            if (UNiagaraSystem* System =
                VisualProfile->MachineGunMuzzleFlash.LoadSynchronous())
            {
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                    this,
                    System,
                    MuzzleLocation,
                    AimDirection.Rotation(),
                    VisualProfile->MachineGunMuzzleScale,
                    true,
                    true,
                    ENCPoolMethod::AutoRelease,
                    true);
            }

            if (USoundBase* Sound =
                VisualProfile->MachineGunSound.LoadSynchronous())
            {
                UGameplayStatics::PlaySoundAtLocation(
                    this,
                    Sound,
                    MuzzleLocation,
                    VisualProfile->MachineGunVolume,
                    1.0f + FMath::FRandRange(-0.025f, 0.025f),
                    0.0f,
                    VisualProfile->Attenuation.LoadSynchronous(),
                    nullptr,
                    nullptr);
            }
        }

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

    if (IsPlayerControlled())
    {
        if (bPlayerMachineGunFiring &&
            Time >= NextMachineGunTime)
        {
            FireMachineGunAt(
                GetPlayerAimPoint(MachineGunRangeCm));

            NextMachineGunTime =
                Time + MachineGunIntervalSeconds;
        }

        return;
    }

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

        const FRotator DesiredTurretRotation =
            (TargetLocation - TurretVisual->GetComponentLocation()).Rotation();

        const float TurretHealthNormalized =
            GetSubsystemHealthNormalized(EWPVehicleSubsystem::Turret);
        const float TurretTurnSpeed =
            FMath::Lerp(0.45f, 2.7f, TurretHealthNormalized);

        TurretVisual->SetWorldRotation(FMath::RInterpTo(
            TurretVisual->GetComponentRotation(),
            FRotator(0.0f, DesiredTurretRotation.Yaw, 0.0f),
            DeltaSeconds,
            TurretTurnSpeed));

        BarrelVisual->SetWorldRotation(FMath::RInterpTo(
            BarrelVisual->GetComponentRotation(),
            DesiredTurretRotation,
            DeltaSeconds,
            TurretTurnSpeed));
    }

    if (Distance > DesiredVehicleRangeCm && !IsImmobilized())
    {
        AddMovementInput(FlatDirection, 0.72f, true);
    }

    const bool bVisible = HasLineOfSightTo(Target);

    if (!bVisible)
    {
        CurrentTarget.Reset();
        return;
    }

    if (Distance <= CannonRangeCm &&
        Time >= NextCannonTime &&
        TurretHealth > 0.0f)
    {
        FireCannon(Target);

        const float TurretHealthNormalized =
            GetSubsystemHealthNormalized(EWPVehicleSubsystem::Turret);

        NextCannonTime =
            Time +
            CannonCooldownSeconds *
                FMath::Lerp(1.75f, 1.0f, TurretHealthNormalized) +
            FMath::FRandRange(0.0f, 1.0f);
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
    const FVector& ShotDirection,
    EWPArmorShellType ShellType)
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

    if (ShellType == EWPArmorShellType::HighExplosive)
    {
        ArmorDamageScale *= 0.72f;
    }

    const float EffectiveDamage = BaseDamage * ArmorDamageScale;
    Health = FMath::Max(0.0f, Health - EffectiveDamage);
    OnVehicleDamaged.Broadcast(EffectiveDamage, GetHealthNormalized());

    const FVector LocalImpact =
        GetActorTransform().InverseTransformPosition(ImpactLocation);

    const float SubsystemDamage =
        EffectiveDamage *
        (ShellType == EWPArmorShellType::ArmorPiercing ? 0.72f : 0.38f);

    if (FMath::Abs(LocalImpact.Y) >= 72.0f &&
        LocalImpact.Z <= 35.0f)
    {
        DamageSubsystem(
            EWPVehicleSubsystem::Tracks,
            SubsystemDamage * 1.2f);
    }

    if (LocalImpact.X <= -70.0f &&
        LocalImpact.Z <= 70.0f)
    {
        DamageSubsystem(
            EWPVehicleSubsystem::Engine,
            SubsystemDamage);
    }

    if (LocalImpact.Z >= 70.0f)
    {
        DamageSubsystem(
            EWPVehicleSubsystem::Turret,
            SubsystemDamage * 0.9f);
    }

    if (FMath::Abs(LocalImpact.Y) >= 58.0f &&
        LocalImpact.X >= -65.0f &&
        LocalImpact.X <= 85.0f &&
        LocalImpact.Z >= 5.0f &&
        LocalImpact.Z <= 80.0f)
    {
        DamageSubsystem(
            EWPVehicleSubsystem::AmmoRack,
            SubsystemDamage * 0.8f);
    }

    UpdateSubsystemEffects();

    if (Health <= 0.0f)
    {
        DestroyVehicle();
    }
}

float AWPArmoredVehicle::GetSubsystemHealthNormalized(
    EWPVehicleSubsystem Subsystem) const
{
    switch (Subsystem)
    {
        case EWPVehicleSubsystem::Engine:
            return EngineMaxHealth > 0.0f
                ? EngineHealth / EngineMaxHealth
                : 0.0f;

        case EWPVehicleSubsystem::Tracks:
            return TrackMaxHealth > 0.0f
                ? TrackHealth / TrackMaxHealth
                : 0.0f;

        case EWPVehicleSubsystem::Turret:
            return TurretMaxHealth > 0.0f
                ? TurretHealth / TurretMaxHealth
                : 0.0f;

        case EWPVehicleSubsystem::AmmoRack:
        default:
            return AmmoRackMaxHealth > 0.0f
                ? AmmoRackHealth / AmmoRackMaxHealth
                : 0.0f;
    }
}

void AWPArmoredVehicle::DamageSubsystem(
    EWPVehicleSubsystem Subsystem,
    float Damage)
{
    if (Damage <= 0.0f || bDestroyed)
    {
        return;
    }

    float* HealthValue = nullptr;
    float MaxValue = 1.0f;

    switch (Subsystem)
    {
        case EWPVehicleSubsystem::Engine:
            HealthValue = &EngineHealth;
            MaxValue = EngineMaxHealth;
            break;

        case EWPVehicleSubsystem::Tracks:
            HealthValue = &TrackHealth;
            MaxValue = TrackMaxHealth;
            break;

        case EWPVehicleSubsystem::Turret:
            HealthValue = &TurretHealth;
            MaxValue = TurretMaxHealth;
            break;

        case EWPVehicleSubsystem::AmmoRack:
            HealthValue = &AmmoRackHealth;
            MaxValue = AmmoRackMaxHealth;
            break;
    }

    if (!HealthValue)
    {
        return;
    }

    const float Previous = *HealthValue;
    *HealthValue = FMath::Max(0.0f, *HealthValue - Damage);

    if (*HealthValue < Previous)
    {
        OnSubsystemDamaged.Broadcast(
            Subsystem,
            MaxValue > 0.0f ? *HealthValue / MaxValue : 0.0f);
    }

    if (Subsystem == EWPVehicleSubsystem::AmmoRack &&
        Previous > 0.0f &&
        *HealthValue <= 0.0f)
    {
        Health = FMath::Max(
            0.0f,
            Health - AmmoRackCatastrophicDamage);

        OnVehicleDamaged.Broadcast(
            AmmoRackCatastrophicDamage,
            GetHealthNormalized());
    }
}

void AWPArmoredVehicle::UpdateSubsystemEffects()
{
    const float EngineFactor =
        FMath::Lerp(
            0.35f,
            1.0f,
            GetSubsystemHealthNormalized(EWPVehicleSubsystem::Engine));

    const float TrackFactor =
        FMath::Lerp(
            0.20f,
            1.0f,
            GetSubsystemHealthNormalized(EWPVehicleSubsystem::Tracks));

    Movement->MaxSpeed =
        (EngineHealth <= 0.0f || TrackHealth <= 0.0f)
            ? 0.0f
            : 500.0f * FMath::Min(EngineFactor, TrackFactor);

    Movement->Acceleration =
        650.0f * FMath::Min(EngineFactor, TrackFactor);
}

void AWPArmoredVehicle::HandleTankShellImpact(FVector ImpactLocation)
{
    if (!VisualProfile)
    {
        return;
    }

    if (UNiagaraSystem* System =
        VisualProfile->ShellImpactEffect.LoadSynchronous())
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            System,
            ImpactLocation,
            FRotator::ZeroRotator,
            VisualProfile->ShellImpactScale,
            true,
            true,
            ENCPoolMethod::AutoRelease,
            true);
    }

    if (USoundBase* Sound =
        VisualProfile->ShellImpactSound.LoadSynchronous())
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            Sound,
            ImpactLocation,
            VisualProfile->ImpactVolume,
            1.0f + FMath::FRandRange(-0.02f, 0.02f),
            0.0f,
            VisualProfile->Attenuation.LoadSynchronous(),
            nullptr,
            nullptr);
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
    bPlayerMachineGunFiring = false;

    if (AWPSoldierCharacter* Soldier = CrewSoldier.Get())
    {
        if (APlayerController* PlayerController =
            Cast<APlayerController>(GetController()))
        {
            PlayerController->Possess(Soldier);
        }

        RestoreCrewSoldier(true);
        CrewSoldier.Reset();
    }

    if (VisualProfile)
    {
        if (UNiagaraSystem* System =
            VisualProfile->DestructionEffect.LoadSynchronous())
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                this,
                System,
                GetActorLocation() + FVector(0.0f, 0.0f, 90.0f),
                GetActorRotation(),
                VisualProfile->DestructionScale,
                true,
                true,
                ENCPoolMethod::AutoRelease,
                true);
        }

        if (USoundBase* Sound =
            VisualProfile->DestructionSound.LoadSynchronous())
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,
                Sound,
                GetActorLocation(),
                VisualProfile->DestructionVolume,
                1.0f + FMath::FRandRange(-0.02f, 0.02f),
                0.0f,
                VisualProfile->Attenuation.LoadSynchronous(),
                nullptr,
                nullptr);
        }
    }

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
