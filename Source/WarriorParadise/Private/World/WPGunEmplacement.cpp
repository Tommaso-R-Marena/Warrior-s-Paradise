#include "World/WPGunEmplacement.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPBallisticProjectile.h"
#include "Combat/WPHealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/WPEmplacementVisualProfile.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/WPArmoredVehicle.h"
#include "Vehicles/WPLightVehicle.h"
#include "Vehicles/WPTankShell.h"

AWPGunEmplacement::AWPGunEmplacement()
{
    PrimaryActorTick.bCanEverTick = true;
    Tags.AddUnique(TEXT("WP_Surface_Metal"));

    CollisionRoot = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionRoot"));
    SetRootComponent(CollisionRoot);
    CollisionRoot->InitBoxExtent(FVector(95.0f, 75.0f, 65.0f));
    CollisionRoot->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    BaseVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseVisual"));
    BaseVisual->SetupAttachment(CollisionRoot);
    BaseVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BaseVisual->SetRelativeScale3D(FVector(1.6f, 1.2f, 0.55f));
    if (CubeAsset.Succeeded())
    {
        BaseVisual->SetStaticMesh(CubeAsset.Object);
    }

    GunVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunVisual"));
    GunVisual->SetupAttachment(CollisionRoot);
    GunVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GunVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
    GunVisual->SetRelativeScale3D(FVector(2.1f, 0.18f, 0.18f));
    if (CubeAsset.Succeeded())
    {
        GunVisual->SetStaticMesh(CubeAsset.Object);
    }

    GunCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("GunCamera"));
    GunCamera->SetupAttachment(GunVisual);
    GunCamera->SetRelativeLocation(FVector(-55.0f, 0.0f, 28.0f));
    GunCamera->bUsePawnControlRotation = false;
}

void AWPGunEmplacement::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    ApplyVisualProfile();

    OnTakeAnyDamage.AddDynamic(
        this,
        &AWPGunEmplacement::HandleTakeAnyDamage);
}

void AWPGunEmplacement::Configure(
    EWPCombatTeam InTeam,
    EWPEmplacementType InType)
{
    CombatTeam = InTeam;
    EmplacementType = InType;

    if (EmplacementType == EWPEmplacementType::AntiTankGun)
    {
        MaxHealth = 420.0f;
        MaximumYawDegrees = 58.0f;
    }
    else
    {
        MaxHealth = 300.0f;
        MaximumYawDegrees = 88.0f;
    }

    Health = MaxHealth;
}

void AWPGunEmplacement::ApplyVisualProfile()
{
    if (!VisualProfile)
    {
        return;
    }

    if (UStaticMesh* Mesh = VisualProfile->BaseMesh.LoadSynchronous())
    {
        BaseVisual->SetStaticMesh(Mesh);
        BaseVisual->SetRelativeTransform(
            VisualProfile->BaseRelativeTransform);
    }

    if (UStaticMesh* Mesh = VisualProfile->GunMesh.LoadSynchronous())
    {
        GunVisual->SetStaticMesh(Mesh);
        GunVisual->SetRelativeTransform(
            VisualProfile->GunRelativeTransform);
    }
}

void AWPGunEmplacement::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(
        TEXT("Turn"),
        this,
        &AWPGunEmplacement::Turn);
    PlayerInputComponent->BindAxis(
        TEXT("LookUp"),
        this,
        &AWPGunEmplacement::LookUp);
    PlayerInputComponent->BindAction(
        TEXT("Fire"),
        IE_Pressed,
        this,
        &AWPGunEmplacement::StartFire);
    PlayerInputComponent->BindAction(
        TEXT("Fire"),
        IE_Released,
        this,
        &AWPGunEmplacement::StopFire);
    PlayerInputComponent->BindAction(
        TEXT("Interact"),
        IE_Pressed,
        this,
        &AWPGunEmplacement::InteractInput);
}

void AWPGunEmplacement::Turn(float Value)
{
    if (!IsPlayerControlled() || bDestroyed)
    {
        return;
    }

    LocalYaw = FMath::Clamp(
        LocalYaw + Value * 1.15f,
        -MaximumYawDegrees,
        MaximumYawDegrees);
    UpdateGunRotation();
}

void AWPGunEmplacement::LookUp(float Value)
{
    if (!IsPlayerControlled() || bDestroyed)
    {
        return;
    }

    LocalPitch = FMath::Clamp(
        LocalPitch + Value * 0.9f,
        MinimumPitchDegrees,
        MaximumPitchDegrees);
    UpdateGunRotation();
}

void AWPGunEmplacement::UpdateGunRotation()
{
    GunVisual->SetRelativeRotation(
        FRotator(LocalPitch, LocalYaw, 0.0f));
}

void AWPGunEmplacement::StartFire()
{
    bPlayerFiring = true;
}

void AWPGunEmplacement::StopFire()
{
    bPlayerFiring = false;
}

void AWPGunEmplacement::InteractInput()
{
    ExitEmplacement();
}

bool AWPGunEmplacement::TryUse(AWPSoldierCharacter* Soldier)
{
    if (!Soldier ||
        bDestroyed ||
        OperatorSoldier.IsValid() ||
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

    OperatorSoldier = Soldier;
    Soldier->GetWeaponComponent()->StopFire();
    Soldier->SetActorHiddenInGame(true);
    Soldier->SetActorEnableCollision(false);
    Soldier->GetCharacterMovement()->DisableMovement();

    PlayerController->Possess(this);
    PlayerController->SetControlRotation(GetActorRotation());
    return true;
}

void AWPGunEmplacement::ExitEmplacement()
{
    AWPSoldierCharacter* Soldier = OperatorSoldier.Get();
    APlayerController* PlayerController =
        Soldier
            ? Cast<APlayerController>(GetController())
            : nullptr;

    if (!Soldier || !PlayerController)
    {
        return;
    }

    PlayerController->Possess(Soldier);
    Soldier->SetActorHiddenInGame(false);
    Soldier->SetActorEnableCollision(true);
    Soldier->SetActorLocation(
        GetActorLocation() -
            GetActorForwardVector() * 130.0f +
            GetActorRightVector() * 120.0f +
            FVector(0.0f, 0.0f, 55.0f),
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
    Soldier->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    OperatorSoldier.Reset();
    bPlayerFiring = false;
}

bool AWPGunEmplacement::HasLineOfSightTo(const AActor* Target) const
{
    if (!Target || !GetWorld())
    {
        return false;
    }

    FHitResult Hit;
    FCollisionQueryParams Params(
        SCENE_QUERY_STAT(WPEmplacementLOS),
        true,
        this);
    Params.AddIgnoredActor(this);

    const FVector Start =
        GunVisual->GetComponentLocation() +
        FVector(0.0f, 0.0f, 10.0f);

    const FVector End =
        Target->GetActorLocation() +
        FVector(0.0f, 0.0f, 50.0f);

    if (!GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility,
        Params))
    {
        return true;
    }

    return Hit.GetActor() == Target;
}

AActor* AWPGunEmplacement::FindBestTarget() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    AActor* BestTarget = nullptr;
    float BestScore = TNumericLimits<float>::Max();
    const float Range =
        EmplacementType == EWPEmplacementType::AntiTankGun
            ? ATRangeCm
            : HMGRangeCm;

    if (EmplacementType == EWPEmplacementType::AntiTankGun)
    {
        for (TActorIterator<AWPArmoredVehicle> It(GetWorld()); It; ++It)
        {
            AWPArmoredVehicle* Vehicle = *It;
            if (!Vehicle ||
                Vehicle->GetCombatTeam() == CombatTeam ||
                Vehicle->IsDestroyed() ||
                !HasLineOfSightTo(Vehicle))
            {
                continue;
            }

            const float Score = FVector::DistSquared2D(
                GetActorLocation(),
                Vehicle->GetActorLocation());

            if (Score < FMath::Square(Range) && Score < BestScore)
            {
                BestScore = Score;
                BestTarget = Vehicle;
            }
        }

        for (TActorIterator<AWPLightVehicle> It(GetWorld()); It; ++It)
        {
            AWPLightVehicle* Vehicle = *It;
            if (!Vehicle ||
                Vehicle->GetCombatTeam() == CombatTeam ||
                Vehicle->IsDestroyed() ||
                !HasLineOfSightTo(Vehicle))
            {
                continue;
            }

            const float Score = FVector::DistSquared2D(
                GetActorLocation(),
                Vehicle->GetActorLocation());

            if (Score < FMath::Square(Range) && Score < BestScore)
            {
                BestScore = Score;
                BestTarget = Vehicle;
            }
        }
    }

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Soldier = *It;
        if (!Soldier ||
            Soldier->GetCombatTeam() == CombatTeam ||
            !Soldier->IsCombatCapable() ||
            !HasLineOfSightTo(Soldier))
        {
            continue;
        }

        float Score = FVector::DistSquared2D(
            GetActorLocation(),
            Soldier->GetActorLocation());

        if (EmplacementType == EWPEmplacementType::AntiTankGun)
        {
            Score *= 1.55f;
        }

        if (Score < FMath::Square(Range) && Score < BestScore)
        {
            BestScore = Score;
            BestTarget = Soldier;
        }
    }

    return BestTarget;
}

void AWPGunEmplacement::FireAt(
    const FVector& TargetLocation,
    AActor* TargetActor)
{
    if (!GetWorld() || bDestroyed)
    {
        return;
    }

    const FVector Muzzle =
        GunVisual->GetComponentLocation() +
        GunVisual->GetForwardVector() * 115.0f;
    const FVector Direction =
        (TargetLocation - Muzzle).GetSafeNormal();

    GunVisual->SetWorldRotation(Direction.Rotation());

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = this;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (EmplacementType == EWPEmplacementType::AntiTankGun)
    {
        if (AWPTankShell* Shell =
            GetWorld()->SpawnActor<AWPTankShell>(
                AWPTankShell::StaticClass(),
                Muzzle,
                Direction.Rotation(),
                SpawnParameters))
        {
            const bool bArmorTarget =
                Cast<AWPArmoredVehicle>(TargetActor) != nullptr ||
                Cast<AWPLightVehicle>(TargetActor) != nullptr;

            Shell->InitializeShell(
                CombatTeam,
                Direction * 24500.0f,
                bArmorTarget
                    ? EWPArmorShellType::ArmorPiercing
                    : EWPArmorShellType::HighExplosive);
        }
    }
    else
    {
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
                    FMath::DegreesToRadians(0.72f)),
                88000.0f,
                23.0f,
                HMGRangeCm,
                nullptr,
                CombatTeam);
        }
    }

    if (VisualProfile)
    {
        if (UNiagaraSystem* System =
            VisualProfile->MuzzleFlash.LoadSynchronous())
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                this,
                System,
                Muzzle,
                Direction.Rotation());
        }

        if (USoundBase* Sound =
            VisualProfile->FireSound.LoadSynchronous())
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,
                Sound,
                Muzzle,
                VisualProfile->FireVolume,
                FMath::FRandRange(0.98f, 1.02f),
                0.0f,
                VisualProfile->Attenuation.LoadSynchronous(),
                nullptr,
                nullptr);
        }
    }

    OnFired.Broadcast();
}

void AWPGunEmplacement::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bDestroyed || !GetWorld())
    {
        return;
    }

    const float Time = GetWorld()->GetTimeSeconds();
    const float Interval =
        EmplacementType == EWPEmplacementType::AntiTankGun
            ? ATFireInterval
            : HMGFireInterval;

    if (IsPlayerControlled())
    {
        if (bPlayerFiring && Time >= NextFireTime)
        {
            const FVector Start = GunCamera->GetComponentLocation();
            const float Range =
                EmplacementType == EWPEmplacementType::AntiTankGun
                    ? ATRangeCm
                    : HMGRangeCm;
            const FVector End =
                Start + GunCamera->GetForwardVector() * Range;

            FHitResult Hit;
            FCollisionQueryParams Params(
                SCENE_QUERY_STAT(WPEmplacementPlayerAim),
                true,
                this);
            Params.AddIgnoredActor(this);

            FVector TargetLocation = End;
            AActor* TargetActor = nullptr;

            if (GetWorld()->LineTraceSingleByChannel(
                Hit,
                Start,
                End,
                ECC_Visibility,
                Params))
            {
                TargetLocation = Hit.ImpactPoint;
                TargetActor = Hit.GetActor();
            }

            FireAt(TargetLocation, TargetActor);
            NextFireTime = Time + Interval;
        }
        return;
    }

    if (!CurrentTarget.IsValid() &&
        Time >= NextTargetScanTime)
    {
        CurrentTarget = FindBestTarget();
        NextTargetScanTime = Time + 0.55f;
    }

    AActor* Target = CurrentTarget.Get();
    if (!Target || !HasLineOfSightTo(Target))
    {
        CurrentTarget.Reset();
        return;
    }

    const FVector TargetLocation =
        Target->GetActorLocation() +
        FVector(0.0f, 0.0f, 50.0f);
    const FVector LocalDirection =
        GetActorTransform()
            .InverseTransformVectorNoScale(
                (TargetLocation - GunVisual->GetComponentLocation())
                    .GetSafeNormal());

    const FRotator Desired = LocalDirection.Rotation();
    LocalYaw = FMath::Clamp(
        Desired.Yaw,
        -MaximumYawDegrees,
        MaximumYawDegrees);
    LocalPitch = FMath::Clamp(
        Desired.Pitch,
        MinimumPitchDegrees,
        MaximumPitchDegrees);
    UpdateGunRotation();

    if (Time >= NextFireTime)
    {
        FireAt(TargetLocation, Target);
        NextFireTime = Time + Interval +
            (EmplacementType == EWPEmplacementType::AntiTankGun
                ? FMath::FRandRange(0.0f, 0.8f)
                : 0.0f);
    }
}

void AWPGunEmplacement::HandleTakeAnyDamage(
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

    Health = FMath::Max(0.0f, Health - Damage);
    if (Health <= 0.0f)
    {
        DestroyEmplacement();
    }
}

void AWPGunEmplacement::DestroyEmplacement()
{
    if (bDestroyed)
    {
        return;
    }

    bDestroyed = true;
    bPlayerFiring = false;

    if (VisualProfile)
    {
        if (UNiagaraSystem* System =
            VisualProfile->DestructionEffect.LoadSynchronous())
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                this,
                System,
                GetActorLocation(),
                GetActorRotation());
        }

        if (USoundBase* Sound =
            VisualProfile->DestructionSound.LoadSynchronous())
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,
                Sound,
                GetActorLocation(),
                VisualProfile->DestructionVolume,
                1.0f,
                0.0f,
                VisualProfile->Attenuation.LoadSynchronous(),
                nullptr,
                nullptr);
        }
    }

    AWPSoldierCharacter* Soldier = OperatorSoldier.Get();
    APlayerController* PlayerController =
        Soldier
            ? Cast<APlayerController>(GetController())
            : nullptr;

    if (Soldier && PlayerController)
    {
        PlayerController->Possess(Soldier);
        Soldier->SetActorHiddenInGame(false);
        Soldier->SetActorEnableCollision(true);
        Soldier->SetActorLocation(
            GetActorLocation() -
                GetActorForwardVector() * 145.0f +
                FVector(0.0f, 0.0f, 55.0f),
            false,
            nullptr,
            ETeleportType::TeleportPhysics);
        Soldier->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

        UGameplayStatics::ApplyDamage(
            Soldier,
            55.0f,
            nullptr,
            this,
            UDamageType::StaticClass());
    }

    OperatorSoldier.Reset();
    OnDestroyed.Broadcast();
    CollisionRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BaseVisual->SetVisibility(false, true);
    GunVisual->SetVisibility(false, true);
    SetLifeSpan(2.0f);
}
