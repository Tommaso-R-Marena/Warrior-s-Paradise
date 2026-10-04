#include "Vehicles/WPLightVehicle.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPBallisticProjectile.h"
#include "Combat/WPHealthComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Game/WPBattlefieldDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/WPLightVehicleVisualProfile.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AWPLightVehicle::AWPLightVehicle()
{
    PrimaryActorTick.bCanEverTick = true;
    Tags.AddUnique(TEXT("WP_Surface_Metal"));

    CollisionRoot = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionRoot"));
    SetRootComponent(CollisionRoot);
    CollisionRoot->InitBoxExtent(FVector(190.0f, 90.0f, 70.0f));
    CollisionRoot->SetCollisionProfileName(TEXT("Pawn"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    BodyVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyVisual"));
    BodyVisual->SetupAttachment(CollisionRoot);
    BodyVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyVisual->SetRelativeScale3D(FVector(3.8f, 1.8f, 1.25f));
    if (CubeAsset.Succeeded())
    {
        BodyVisual->SetStaticMesh(CubeAsset.Object);
    }

    WeaponVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponVisual"));
    WeaponVisual->SetupAttachment(CollisionRoot);
    WeaponVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponVisual->SetRelativeLocation(FVector(35.0f, 0.0f, 110.0f));
    WeaponVisual->SetRelativeScale3D(FVector(1.2f, 0.18f, 0.18f));
    WeaponVisual->SetVisibility(false, true);
    if (CubeAsset.Succeeded())
    {
        WeaponVisual->SetStaticMesh(CubeAsset.Object);
    }

    DriverCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("DriverCamera"));
    DriverCamera->SetupAttachment(CollisionRoot);
    DriverCamera->SetRelativeLocation(FVector(-70.0f, 0.0f, 165.0f));
    DriverCamera->bUsePawnControlRotation = true;

    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->UpdatedComponent = CollisionRoot;
    Movement->MaxSpeed = 850.0f;
    Movement->Acceleration = 850.0f;
    Movement->Deceleration = 1000.0f;
    Movement->TurningBoost = 5.0f;
}

UPawnMovementComponent* AWPLightVehicle::GetMovementComponent() const
{
    return Movement;
}

void AWPLightVehicle::BeginPlay()
{
    Super::BeginPlay();

    SetVehicleType(VehicleType);
    Health = MaxHealth;
    ApplyVisualProfile();

    BattlefieldDirector = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(
            this,
            AWPBattlefieldDirector::StaticClass()));

    OnTakeAnyDamage.AddDynamic(
        this,
        &AWPLightVehicle::HandleTakeAnyDamage);

    if (VisualProfile)
    {
        if (USoundBase* Engine =
            VisualProfile->EngineLoop.LoadSynchronous())
        {
            EngineAudioComponent = UGameplayStatics::SpawnSoundAttached(
                Engine,
                CollisionRoot,
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
    }
}

void AWPLightVehicle::SetVehicleType(EWPLightVehicleType NewType)
{
    VehicleType = NewType;

    if (VehicleType == EWPLightVehicleType::ArmoredCar)
    {
        MaxHealth = 580.0f;
        PassengerCapacity = 2;
        Movement->MaxSpeed = 1050.0f;
        WeaponVisual->SetVisibility(true, true);
    }
    else
    {
        MaxHealth = 420.0f;
        PassengerCapacity = 5;
        Movement->MaxSpeed = 850.0f;
        WeaponVisual->SetVisibility(false, true);
    }

    Health = FMath::Min(Health, MaxHealth);
}

void AWPLightVehicle::ApplyVisualProfile()
{
    if (!VisualProfile)
    {
        return;
    }

    if (UStaticMesh* Mesh = VisualProfile->BodyMesh.LoadSynchronous())
    {
        BodyVisual->SetStaticMesh(Mesh);
        BodyVisual->SetRelativeTransform(
            VisualProfile->BodyRelativeTransform);
    }

    if (UStaticMesh* Mesh = VisualProfile->WeaponMesh.LoadSynchronous())
    {
        WeaponVisual->SetStaticMesh(Mesh);
        WeaponVisual->SetRelativeTransform(
            VisualProfile->WeaponRelativeTransform);
        WeaponVisual->SetVisibility(
            VehicleType == EWPLightVehicleType::ArmoredCar,
            true);
    }
}

void AWPLightVehicle::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(
        TEXT("MoveForward"),
        this,
        &AWPLightVehicle::MoveForward);
    PlayerInputComponent->BindAxis(
        TEXT("Turn"),
        this,
        &AWPLightVehicle::Turn);
    PlayerInputComponent->BindAction(
        TEXT("Fire"),
        IE_Pressed,
        this,
        &AWPLightVehicle::FireInput);
    PlayerInputComponent->BindAction(
        TEXT("Interact"),
        IE_Pressed,
        this,
        &AWPLightVehicle::InteractInput);
}

void AWPLightVehicle::MoveForward(float Value)
{
    if (bDestroyed || FMath::IsNearlyZero(Value))
    {
        return;
    }

    AddMovementInput(GetActorForwardVector(), Value, true);
}

void AWPLightVehicle::Turn(float Value)
{
    SteeringInput = Value;

    if (bDestroyed || FMath::IsNearlyZero(Value))
    {
        return;
    }

    FRotator Rotation = GetActorRotation();
    Rotation.Yaw += Value * 1.8f;
    SetActorRotation(Rotation);
}

void AWPLightVehicle::FireInput()
{
    if (bDestroyed ||
        VehicleType != EWPLightVehicleType::ArmoredCar ||
        !GetWorld())
    {
        return;
    }

    FVector Start = WeaponVisual->GetComponentLocation() +
        WeaponVisual->GetForwardVector() * 100.0f;
    FVector End = Start + DriverCamera->GetForwardVector() * MachineGunRangeCm;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WPLightVehiclePlayerAim), true, this);
    Params.AddIgnoredActor(this);

    FVector AimPoint = End;
    if (GetWorld()->LineTraceSingleByChannel(
        Hit,
        DriverCamera->GetComponentLocation(),
        End,
        ECC_Visibility,
        Params))
    {
        AimPoint = Hit.ImpactPoint;
    }

    FireMachineGun(nullptr);

    const FVector Direction = (AimPoint - Start).GetSafeNormal();
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = this;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AWPBallisticProjectile* Projectile =
        GetWorld()->SpawnActor<AWPBallisticProjectile>(
            AWPBallisticProjectile::StaticClass(),
            Start,
            Direction.Rotation(),
            SpawnParameters))
    {
        Projectile->InitializeProjectile(
            Direction,
            82000.0f,
            20.0f,
            MachineGunRangeCm,
            nullptr,
            CombatTeam);
    }
}

void AWPLightVehicle::InteractInput()
{
    ExitVehicle();
}

bool AWPLightVehicle::TryEnter(AWPSoldierCharacter* Soldier)
{
    if (!Soldier ||
        bDestroyed ||
        DriverSoldier.IsValid() ||
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

    DriverSoldier = Soldier;
    Soldier->GetWeaponComponent()->StopFire();
    Soldier->SetActorHiddenInGame(true);
    Soldier->SetActorEnableCollision(false);
    Soldier->GetCharacterMovement()->DisableMovement();

    EmbarkNearbySquad(Soldier);

    PlayerController->Possess(this);
    PlayerController->SetControlRotation(GetActorRotation());
    return true;
}

void AWPLightVehicle::EmbarkNearbySquad(AWPSoldierCharacter* Driver)
{
    if (!GetWorld() || PassengerCapacity <= 0)
    {
        return;
    }

    Passengers.Reset();

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld());
         It && Passengers.Num() < PassengerCapacity;
         ++It)
    {
        AWPSoldierCharacter* Candidate = *It;
        if (!Candidate ||
            Candidate == Driver ||
            Candidate->GetCombatTeam() != CombatTeam ||
            Candidate->IsPlayerControlled() ||
            !Candidate->IsCombatCapable() ||
            FVector::DistSquared2D(
                GetActorLocation(),
                Candidate->GetActorLocation()) >
                FMath::Square(EmbarkRadiusCm))
        {
            continue;
        }

        Candidate->GetWeaponComponent()->StopFire();
        Candidate->SetActorHiddenInGame(true);
        Candidate->SetActorEnableCollision(false);
        Candidate->GetCharacterMovement()->DisableMovement();
        Passengers.Add(Candidate);
    }
}

void AWPLightVehicle::RestoreSoldierFromVehicle(
    AWPSoldierCharacter* Soldier,
    const FVector& ExitLocation,
    bool bEmergency)
{
    if (!Soldier || !Soldier->GetHealthComponent()->IsDead())
    {
        if (!Soldier)
        {
            return;
        }
    }

    Soldier->SetActorHiddenInGame(false);
    Soldier->SetActorEnableCollision(true);
    Soldier->SetActorLocation(
        ExitLocation,
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
                45.0f,
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }
}

void AWPLightVehicle::DisembarkPassengers(bool bEmergency)
{
    int32 Index = 0;
    for (TWeakObjectPtr<AWPSoldierCharacter>& PassengerPtr : Passengers)
    {
        AWPSoldierCharacter* Passenger = PassengerPtr.Get();
        if (!Passenger)
        {
            continue;
        }

        const float Side = (Index % 2 == 0) ? 1.0f : -1.0f;
        const float Row = static_cast<float>(Index / 2);
        const FVector ExitLocation =
            GetActorLocation() -
            GetActorForwardVector() * (180.0f + Row * 90.0f) +
            GetActorRightVector() * Side * 170.0f +
            FVector(0.0f, 0.0f, 60.0f);

        RestoreSoldierFromVehicle(
            Passenger,
            ExitLocation,
            bEmergency);
        ++Index;
    }

    Passengers.Reset();
}

void AWPLightVehicle::ExitVehicle()
{
    AWPSoldierCharacter* Driver = DriverSoldier.Get();
    APlayerController* PlayerController =
        Driver
            ? Cast<APlayerController>(GetController())
            : nullptr;

    if (!Driver || !PlayerController)
    {
        return;
    }

    const FVector ExitLocation =
        GetActorLocation() +
        GetActorRightVector() * 210.0f +
        FVector(0.0f, 0.0f, 60.0f);

    PlayerController->Possess(Driver);
    RestoreSoldierFromVehicle(
        Driver,
        ExitLocation,
        false);
    DriverSoldier.Reset();

    DisembarkPassengers(false);
}

AActor* AWPLightVehicle::FindBestTarget() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    AWPSoldierCharacter* BestTarget = nullptr;
    float BestDistanceSquared = FMath::Square(MachineGunRangeCm);

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Soldier = *It;
        if (!Soldier ||
            Soldier->GetCombatTeam() == CombatTeam ||
            !Soldier->IsCombatCapable())
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared2D(
            GetActorLocation(),
            Soldier->GetActorLocation());

        if (DistanceSquared >= BestDistanceSquared)
        {
            continue;
        }

        FHitResult Hit;
        FCollisionQueryParams Params(
            SCENE_QUERY_STAT(WPLightVehicleTargetLOS),
            true,
            this);
        Params.AddIgnoredActor(this);

        const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
            Hit,
            WeaponVisual->GetComponentLocation(),
            Soldier->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f),
            ECC_Visibility,
            Params);

        if (!bBlocked || Hit.GetActor() == Soldier)
        {
            BestDistanceSquared = DistanceSquared;
            BestTarget = Soldier;
        }
    }

    return BestTarget;
}

void AWPLightVehicle::FireMachineGun(AActor* Target)
{
    if (!GetWorld() ||
        VehicleType != EWPLightVehicleType::ArmoredCar)
    {
        return;
    }

    if (Target)
    {
        const FVector Direction =
            (Target->GetActorLocation() +
             FVector(0.0f, 0.0f, 45.0f) -
             WeaponVisual->GetComponentLocation())
                .GetSafeNormal();

        WeaponVisual->SetWorldRotation(Direction.Rotation());

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = this;
        SpawnParameters.Instigator = this;
        SpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        if (AWPBallisticProjectile* Projectile =
            GetWorld()->SpawnActor<AWPBallisticProjectile>(
                AWPBallisticProjectile::StaticClass(),
                WeaponVisual->GetComponentLocation() +
                    Direction * 90.0f,
                Direction.Rotation(),
                SpawnParameters))
        {
            Projectile->InitializeProjectile(
                FMath::VRandCone(
                    Direction,
                    FMath::DegreesToRadians(1.15f)),
                82000.0f,
                20.0f,
                MachineGunRangeCm,
                nullptr,
                CombatTeam);
        }
    }

    if (VisualProfile)
    {
        const FVector Muzzle =
            WeaponVisual->GetComponentLocation() +
            WeaponVisual->GetForwardVector() * 90.0f;

        if (UNiagaraSystem* System =
            VisualProfile->MachineGunMuzzleFlash.LoadSynchronous())
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                this,
                System,
                Muzzle,
                WeaponVisual->GetComponentRotation());
        }

        if (USoundBase* Sound =
            VisualProfile->MachineGunSound.LoadSynchronous())
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,
                Sound,
                Muzzle,
                VisualProfile->WeaponVolume,
                FMath::FRandRange(0.97f, 1.03f),
                0.0f,
                VisualProfile->Attenuation.LoadSynchronous(),
                nullptr,
                nullptr);
        }
    }
}

void AWPLightVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bDestroyed ||
        !GetWorld() ||
        IsPlayerControlled() ||
        VehicleType != EWPLightVehicleType::ArmoredCar)
    {
        return;
    }

    const float Time = GetWorld()->GetTimeSeconds();

    if (!CurrentTarget.IsValid() &&
        Time >= NextTargetSearchTime)
    {
        CurrentTarget = FindBestTarget();
        NextTargetSearchTime = Time + 0.55f;
    }

    AActor* Target = CurrentTarget.Get();

    if (!Target)
    {
        AWPBattlefieldDirector* Director = BattlefieldDirector.Get();
        if (Director)
        {
            FVector Direction =
                Director->GetActiveObjectiveLocation() -
                GetActorLocation();
            Direction.Z = 0.0f;

            if (Direction.SizeSquared() > FMath::Square(600.0f))
            {
                Direction.Normalize();
                SetActorRotation(
                    FMath::RInterpTo(
                        GetActorRotation(),
                        Direction.Rotation(),
                        DeltaSeconds,
                        1.5f));
                AddMovementInput(Direction, 0.72f, true);
            }
        }
        return;
    }

    AWPSoldierCharacter* Soldier =
        Cast<AWPSoldierCharacter>(Target);

    if (!Soldier || !Soldier->IsCombatCapable())
    {
        CurrentTarget.Reset();
        return;
    }

    FVector ToTarget =
        Soldier->GetActorLocation() - GetActorLocation();
    const float Distance = ToTarget.Size2D();
    ToTarget.Z = 0.0f;
    ToTarget.Normalize();

    SetActorRotation(
        FMath::RInterpTo(
            GetActorRotation(),
            ToTarget.Rotation(),
            DeltaSeconds,
            1.8f));

    if (Distance > DesiredCombatRangeCm)
    {
        AddMovementInput(ToTarget, 0.6f, true);
    }

    if (Distance <= MachineGunRangeCm &&
        Time >= NextMachineGunTime)
    {
        FireMachineGun(Target);
        NextMachineGunTime =
            Time + MachineGunIntervalSeconds;
    }
}

void AWPLightVehicle::HandleTakeAnyDamage(
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

    const float EffectiveDamage =
        VehicleType == EWPLightVehicleType::ArmoredCar
            ? (Damage < 16.0f ? 0.0f : Damage * 0.85f)
            : Damage;

    Health = FMath::Max(0.0f, Health - EffectiveDamage);

    if (Health <= 0.0f)
    {
        DestroyVehicle();
    }
}

void AWPLightVehicle::DestroyVehicle()
{
    if (bDestroyed)
    {
        return;
    }

    bDestroyed = true;

    if (EngineAudioComponent)
    {
        EngineAudioComponent->Stop();
    }

    if (VisualProfile)
    {
        if (UNiagaraSystem* System =
            VisualProfile->DestructionEffect.LoadSynchronous())
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                this,
                System,
                GetActorLocation() + FVector(0.0f, 0.0f, 70.0f),
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

    APlayerController* PlayerController =
        Cast<APlayerController>(GetController());
    AWPSoldierCharacter* Driver = DriverSoldier.Get();

    if (Driver && PlayerController)
    {
        PlayerController->Possess(Driver);
        RestoreSoldierFromVehicle(
            Driver,
            GetActorLocation() +
                GetActorRightVector() * 240.0f +
                FVector(0.0f, 0.0f, 70.0f),
            true);
    }

    DriverSoldier.Reset();
    DisembarkPassengers(true);

    OnVehicleDestroyed.Broadcast();

    if (AWPBattlefieldDirector* Director =
        BattlefieldDirector.Get())
    {
        Director->NotifyVehicleDestroyed(CombatTeam);
    }

    CollisionRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyVisual->SetVisibility(false, true);
    WeaponVisual->SetVisibility(false, true);
    SetLifeSpan(2.0f);
}
