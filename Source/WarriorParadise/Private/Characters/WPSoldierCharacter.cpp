#include "Characters/WPSoldierCharacter.h"
#include "Camera/CameraComponent.h"
#include "Combat/WPFragGrenade.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSmokeGrenade.h"
#include "Combat/WPSuppressionComponent.h"
#include "Combat/WPWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Game/WPBattlefieldDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Presentation/WPInfantryVisualProfile.h"
#include "Presentation/WPWeaponPresentationComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AWPSoldierCharacter::AWPSoldierCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 94.0f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 255.0f;
    GetCharacterMovement()->AirControl = 0.24f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1750.0f;
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 360.0f;

    bUseControllerRotationYaw = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(-8.0f, 0.0f, 68.0f));
    FirstPersonCamera->SetFieldOfView(HipFOV);
    FirstPersonCamera->bUsePawnControlRotation = true;

    FirstPersonArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
    FirstPersonArms->SetupAttachment(FirstPersonCamera);
    FirstPersonArms->SetOnlyOwnerSee(true);
    FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FirstPersonArms->SetCastShadow(false);
    FirstPersonArms->SetVisibility(false, true);

    FirstPersonWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonWeapon"));
    FirstPersonWeapon->SetupAttachment(FirstPersonArms);
    FirstPersonWeapon->SetOnlyOwnerSee(true);
    FirstPersonWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FirstPersonWeapon->SetCastShadow(false);
    FirstPersonWeapon->SetVisibility(false, true);

    FirstPersonStaticWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonStaticWeapon"));
    FirstPersonStaticWeapon->SetupAttachment(FirstPersonArms);
    FirstPersonStaticWeapon->SetOnlyOwnerSee(true);
    FirstPersonStaticWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FirstPersonStaticWeapon->SetCastShadow(false);
    FirstPersonStaticWeapon->SetVisibility(false, true);

    ThirdPersonWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThirdPersonWeapon"));
    ThirdPersonWeapon->SetupAttachment(GetMesh());
    ThirdPersonWeapon->SetOwnerNoSee(true);
    ThirdPersonWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ThirdPersonWeapon->SetVisibility(false, true);

    MuzzleAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleAnchor"));
    MuzzleAnchor->SetupAttachment(FirstPersonCamera);
    MuzzleAnchor->SetRelativeLocation(FVector(90.0f, 0.0f, -10.0f));

    GetMesh()->SetOwnerNoSee(true);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    HealthComponent = CreateDefaultSubobject<UWPHealthComponent>(TEXT("HealthComponent"));
    WeaponComponent = CreateDefaultSubobject<UWPWeaponComponent>(TEXT("WeaponComponent"));
    SuppressionComponent = CreateDefaultSubobject<UWPSuppressionComponent>(TEXT("SuppressionComponent"));
    WeaponPresentationComponent = CreateDefaultSubobject<UWPWeaponPresentationComponent>(
        TEXT("WeaponPresentationComponent"));

    GrenadeClass = AWPFragGrenade::StaticClass();
    SmokeGrenadeClass = AWPSmokeGrenade::StaticClass();

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    ViewWeaponBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ViewWeaponBody"));
    ViewWeaponBody->SetupAttachment(FirstPersonCamera);
    ViewWeaponBody->SetOnlyOwnerSee(true);
    ViewWeaponBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ViewWeaponBody->CastShadow = false;
    ViewWeaponBody->SetRelativeLocation(HipBodyLocation);
    ViewWeaponBody->SetRelativeScale3D(FVector(0.45f, 0.08f, 0.09f));
    if (CubeAsset.Succeeded())
    {
        ViewWeaponBody->SetStaticMesh(CubeAsset.Object);
    }

    ViewWeaponBarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ViewWeaponBarrel"));
    ViewWeaponBarrel->SetupAttachment(FirstPersonCamera);
    ViewWeaponBarrel->SetOnlyOwnerSee(true);
    ViewWeaponBarrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ViewWeaponBarrel->CastShadow = false;
    ViewWeaponBarrel->SetRelativeLocation(HipBarrelLocation);
    ViewWeaponBarrel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    ViewWeaponBarrel->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.42f));
    if (CylinderAsset.Succeeded())
    {
        ViewWeaponBarrel->SetStaticMesh(CylinderAsset.Object);
    }
}

void AWPSoldierCharacter::BeginPlay()
{
    Super::BeginPlay();

    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    FirstPersonCamera->SetFieldOfView(HipFOV);
    Stamina = MaxStamina;
    ApplyRoleLoadout();
    ApplyVisualProfile();

    HealthComponent->OnDowned.AddDynamic(this, &AWPSoldierCharacter::HandleDowned);
    HealthComponent->OnRevived.AddDynamic(this, &AWPSoldierCharacter::HandleRevived);
    HealthComponent->OnDeath.AddDynamic(this, &AWPSoldierCharacter::HandleDeath);
    WeaponComponent->OnShotFired.AddDynamic(this, &AWPSoldierCharacter::HandleShotFired);
}

void AWPSoldierCharacter::ApplyVisualProfile()
{
    bProductionVisualsActive = false;

    GetMesh()->SetVisibility(false, true);
    FirstPersonArms->SetVisibility(false, true);
    FirstPersonWeapon->SetVisibility(false, true);
    FirstPersonStaticWeapon->SetVisibility(false, true);
    ThirdPersonWeapon->SetVisibility(false, true);

    ViewWeaponBody->SetVisibility(true, true);
    ViewWeaponBarrel->SetVisibility(true, true);

    MuzzleAnchor->AttachToComponent(
        FirstPersonCamera,
        FAttachmentTransformRules::KeepRelativeTransform);
    MuzzleAnchor->SetRelativeTransform(FTransform(
        FRotator::ZeroRotator,
        FVector(90.0f, 0.0f, -10.0f),
        FVector::OneVector));

    if (!VisualProfile)
    {
        return;
    }

    if (USkeletalMesh* ThirdPersonMesh = VisualProfile->ThirdPersonMesh.LoadSynchronous())
    {
        GetMesh()->SetSkeletalMesh(ThirdPersonMesh);
        GetMesh()->SetRelativeLocation(VisualProfile->ThirdPersonRelativeLocation);
        GetMesh()->SetRelativeRotation(VisualProfile->ThirdPersonRelativeRotation);
        GetMesh()->SetRelativeScale3D(VisualProfile->ThirdPersonRelativeScale);
        GetMesh()->SetOwnerNoSee(true);
        GetMesh()->SetVisibility(true, true);

        if (VisualProfile->ThirdPersonAnimClass)
        {
            GetMesh()->SetAnimInstanceClass(VisualProfile->ThirdPersonAnimClass);
        }

        bProductionVisualsActive = true;
    }

    if (UStaticMesh* ThirdPersonWeaponMesh =
        VisualProfile->ThirdPersonWeaponMesh.LoadSynchronous())
    {
        ThirdPersonWeapon->SetStaticMesh(ThirdPersonWeaponMesh);
        ThirdPersonWeapon->AttachToComponent(
            GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            VisualProfile->ThirdPersonWeaponAttachSocket);
        ThirdPersonWeapon->SetRelativeTransform(
            VisualProfile->ThirdPersonWeaponRelativeTransform);
        ThirdPersonWeapon->SetVisibility(true, true);
        bProductionVisualsActive = true;
    }

    if (USkeletalMesh* ArmsMesh = VisualProfile->FirstPersonArmsMesh.LoadSynchronous())
    {
        FirstPersonArms->SetSkeletalMesh(ArmsMesh);
        FirstPersonArms->SetRelativeLocation(VisualProfile->FirstPersonArmsRelativeLocation);
        FirstPersonArms->SetRelativeRotation(VisualProfile->FirstPersonArmsRelativeRotation);
        FirstPersonArms->SetRelativeScale3D(VisualProfile->FirstPersonArmsRelativeScale);
        FirstPersonArms->SetVisibility(true, true);

        if (VisualProfile->FirstPersonArmsAnimClass)
        {
            FirstPersonArms->SetAnimInstanceClass(VisualProfile->FirstPersonArmsAnimClass);
        }

        bProductionVisualsActive = true;
    }

    USceneComponent* ActiveFirstPersonWeapon = nullptr;

    if (USkeletalMesh* WeaponMesh = VisualProfile->FirstPersonWeaponMesh.LoadSynchronous())
    {
        FirstPersonWeapon->SetSkeletalMesh(WeaponMesh);

        if (FirstPersonArms->GetSkeletalMeshAsset() &&
            VisualProfile->WeaponAttachSocket != NAME_None)
        {
            FirstPersonWeapon->AttachToComponent(
                FirstPersonArms,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                VisualProfile->WeaponAttachSocket);
        }
        else
        {
            FirstPersonWeapon->AttachToComponent(
                FirstPersonCamera,
                FAttachmentTransformRules::KeepRelativeTransform);
        }

        FirstPersonWeapon->SetRelativeTransform(
            VisualProfile->FirstPersonWeaponRelativeTransform);
        FirstPersonWeapon->SetVisibility(true, true);
        ActiveFirstPersonWeapon = FirstPersonWeapon;
        bProductionVisualsActive = true;
    }
    else if (UStaticMesh* StaticWeaponMesh =
        VisualProfile->FirstPersonStaticWeaponMesh.LoadSynchronous())
    {
        FirstPersonStaticWeapon->SetStaticMesh(StaticWeaponMesh);

        if (FirstPersonArms->GetSkeletalMeshAsset() &&
            VisualProfile->WeaponAttachSocket != NAME_None)
        {
            FirstPersonStaticWeapon->AttachToComponent(
                FirstPersonArms,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                VisualProfile->WeaponAttachSocket);
        }
        else
        {
            FirstPersonStaticWeapon->AttachToComponent(
                FirstPersonCamera,
                FAttachmentTransformRules::KeepRelativeTransform);
        }

        FirstPersonStaticWeapon->SetRelativeTransform(
            VisualProfile->FirstPersonWeaponRelativeTransform);
        FirstPersonStaticWeapon->SetVisibility(true, true);
        ActiveFirstPersonWeapon = FirstPersonStaticWeapon;
        bProductionVisualsActive = true;
    }

    if (ActiveFirstPersonWeapon &&
        VisualProfile->MuzzleAttachSocket != NAME_None)
    {
        MuzzleAnchor->AttachToComponent(
            ActiveFirstPersonWeapon,
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            VisualProfile->MuzzleAttachSocket);
        MuzzleAnchor->SetRelativeTransform(VisualProfile->MuzzleRelativeTransform);
    }

    if (bProductionVisualsActive)
    {
        ViewWeaponBody->SetVisibility(false, true);
        ViewWeaponBarrel->SetVisibility(false, true);
    }
}

FTransform AWPSoldierCharacter::GetWeaponPresentationTransform() const
{
    return MuzzleAnchor
        ? MuzzleAnchor->GetComponentTransform()
        : GetActorTransform();
}

void AWPSoldierCharacter::ApplyRoleLoadout()
{
    switch (SoldierRole)
    {
        case EWPSoldierRole::Assault:
            WeaponComponent->Configure(23.0f, 720.0f, 1.45f);
            GrenadesRemaining = FMath::Max(GrenadesRemaining, 1);
            break;

        case EWPSoldierRole::Medic:
            WeaponComponent->Configure(30.0f, 450.0f, 1.12f);
            GrenadesRemaining = FMath::Min(GrenadesRemaining, 1);
            MedkitsRemaining = FMath::Max(MedkitsRemaining, 2);
            break;

        case EWPSoldierRole::Engineer:
            WeaponComponent->Configure(32.0f, 500.0f, 1.08f);
            GrenadesRemaining = FMath::Max(GrenadesRemaining, 2);
            break;

        case EWPSoldierRole::Rifleman:
        default:
            WeaponComponent->Configure(34.0f, 500.0f, 0.95f);
            GrenadesRemaining = FMath::Max(GrenadesRemaining, 2);
            break;
    }
}

bool AWPSoldierCharacter::IsDowned() const
{
    return HealthComponent && HealthComponent->IsDowned();
}

bool AWPSoldierCharacter::IsCombatCapable() const
{
    return HealthComponent && !HealthComponent->IsDead() && !HealthComponent->IsDowned();
}

AWPBattlefieldDirector* AWPSoldierCharacter::GetBattlefieldDirector() const
{
    return Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass()));
}

bool AWPSoldierCharacter::GetViewTarget(FVector& OutTargetLocation, float MaxRangeCm) const
{
    if (!GetWorld())
    {
        return false;
    }

    FVector ViewLocation = GetActorLocation() + FVector(0.0f, 0.0f, BaseEyeHeight);
    FRotator ViewRotation = GetActorRotation();

    if (Controller)
    {
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
    }

    const FVector End = ViewLocation + ViewRotation.Vector() * MaxRangeCm;
    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WPViewTarget), true, this);
    QueryParams.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, End, ECC_Visibility, QueryParams))
    {
        OutTargetLocation = Hit.ImpactPoint;
    }
    else
    {
        OutTargetLocation = End;
    }

    return true;
}

bool AWPSoldierCharacter::ThrowGrenadeAt(const FVector& TargetLocation)
{
    if (!GetWorld() ||
        !GrenadeClass ||
        GrenadesRemaining <= 0 ||
        !IsCombatCapable())
    {
        return false;
    }

    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, BaseEyeHeight - 12.0f);
    FVector ToTarget = TargetLocation - Start;
    const float HorizontalDistance = FVector(ToTarget.X, ToTarget.Y, 0.0f).Size();

    FVector LaunchDirection = ToTarget.GetSafeNormal();
    FVector LaunchVelocity = LaunchDirection * GrenadeThrowSpeed;
    LaunchVelocity.Z += FMath::Clamp(420.0f + HorizontalDistance * 0.10f, 420.0f, 760.0f);

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = this;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AWPFragGrenade* Grenade = GetWorld()->SpawnActor<AWPFragGrenade>(
        GrenadeClass,
        Start + GetActorForwardVector() * 45.0f,
        LaunchVelocity.Rotation(),
        SpawnParameters))
    {
        Grenade->InitializeGrenade(CombatTeam, LaunchVelocity);
        --GrenadesRemaining;
        return true;
    }

    return false;
}

bool AWPSoldierCharacter::ThrowSmokeGrenadeAt(const FVector& TargetLocation)
{
    if (!GetWorld() ||
        !SmokeGrenadeClass ||
        SmokeGrenadesRemaining <= 0 ||
        !IsCombatCapable())
    {
        return false;
    }

    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, BaseEyeHeight - 12.0f);
    const FVector ToTarget = TargetLocation - Start;
    const float HorizontalDistance = FVector(ToTarget.X, ToTarget.Y, 0.0f).Size();

    FVector LaunchVelocity = ToTarget.GetSafeNormal() * (GrenadeThrowSpeed * 0.9f);
    LaunchVelocity.Z += FMath::Clamp(390.0f + HorizontalDistance * 0.09f, 390.0f, 700.0f);

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = this;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AWPSmokeGrenade* Grenade = GetWorld()->SpawnActor<AWPSmokeGrenade>(
        SmokeGrenadeClass,
        Start + GetActorForwardVector() * 45.0f,
        LaunchVelocity.Rotation(),
        SpawnParameters))
    {
        Grenade->InitializeSmokeGrenade(LaunchVelocity);
        --SmokeGrenadesRemaining;
        return true;
    }

    return false;
}

bool AWPSoldierCharacter::TryReviveSoldier(AWPSoldierCharacter* Target)
{
    if (!Target ||
        Target == this ||
        Target->GetCombatTeam() != CombatTeam ||
        !IsCombatCapable() ||
        !Target->GetHealthComponent() ||
        !Target->GetHealthComponent()->IsDowned())
    {
        return false;
    }

    if (FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) >
        FMath::Square(ReviveInteractionRange))
    {
        return false;
    }

    return Target->GetHealthComponent()->Revive();
}

bool AWPSoldierCharacter::TryMeleeAttack(AWPSoldierCharacter* Target)
{
    if (!Target ||
        Target == this ||
        Target->GetCombatTeam() == CombatTeam ||
        !Target->GetHealthComponent() ||
        Target->GetHealthComponent()->IsDead() ||
        !IsCombatCapable() ||
        !GetWorld())
    {
        return false;
    }

    const float Time = GetWorld()->GetTimeSeconds();
    if (Time < NextMeleeTime ||
        FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) >
            FMath::Square(MeleeRange))
    {
        return false;
    }

    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WPMeleeLOS), true, this);
    Params.AddIgnoredActor(this);

    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility,
        Params) &&
        Hit.GetActor() != Target;

    if (bBlocked)
    {
        return false;
    }

    CancelBandage();
    StopSprint();
    StopAim();
    WeaponComponent->StopFire();
    WeaponComponent->CancelReload();

    NextMeleeTime = Time + MeleeCooldownSeconds;
    OnMeleeAttack.Broadcast();

    UGameplayStatics::ApplyDamage(
        Target,
        MeleeDamage,
        GetController(),
        this,
        UDamageType::StaticClass());

    return true;
}

void AWPSoldierCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!IsPlayerControlled())
    {
        return;
    }

    const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const float Speed2D = GetVelocity().Size2D();
    const float SpeedAlpha = FMath::Clamp(
        Speed2D / FMath::Max(1.0f, SprintSpeed),
        0.0f,
        1.0f);

    if (bSprinting && IsCombatCapable() && Speed2D > 25.0f)
    {
        Stamina = FMath::Max(0.0f, Stamina - SprintDrainPerSecond * DeltaSeconds);
        LastSprintActivityTime = Time;

        if (Stamina <= KINDA_SMALL_NUMBER)
        {
            StopSprint();
        }
    }
    else if (Time - LastSprintActivityTime >= StaminaRegenDelay)
    {
        Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSecond * DeltaSeconds);
    }

    const float Suppression = SuppressionComponent
        ? SuppressionComponent->GetNormalizedSuppression()
        : 0.0f;

    if (IsCombatCapable())
    {
        const float RecoveryScale = FMath::Lerp(1.0f, 0.72f, Suppression);
        const float RecoverySpeed = RecoilRecoverySpeed * RecoveryScale;

        const float NewPitch = FMath::FInterpTo(
            PendingRecoilPitch,
            0.0f,
            DeltaSeconds,
            RecoverySpeed);
        const float PitchRecovered = PendingRecoilPitch - NewPitch;
        if (!FMath::IsNearlyZero(PitchRecovered))
        {
            AddControllerPitchInput(PitchRecovered);
        }
        PendingRecoilPitch = NewPitch;

        const float NewYaw = FMath::FInterpTo(
            PendingRecoilYaw,
            0.0f,
            DeltaSeconds,
            RecoverySpeed);
        const float YawRecovered = PendingRecoilYaw - NewYaw;
        if (!FMath::IsNearlyZero(YawRecovered))
        {
            AddControllerYawInput(-YawRecovered);
        }
        PendingRecoilYaw = NewYaw;
    }

    float AccuracyMultiplier = 1.0f;
    AccuracyMultiplier *= FMath::Lerp(1.0f, 2.0f, SpeedAlpha);
    AccuracyMultiplier *= FMath::Lerp(1.0f, 1.65f, Suppression);

    if (bProne)
    {
        AccuracyMultiplier *= 0.50f;
    }
    else if (bIsCrouched)
    {
        AccuracyMultiplier *= 0.72f;
    }

    if (bSprinting)
    {
        AccuracyMultiplier *= 1.35f;
    }

    WeaponComponent->SetAccuracyMultiplier(AccuracyMultiplier);

    const float BaseTargetFOV = bAiming ? AimFOV : HipFOV;
    const float TargetFOV = BaseTargetFOV - Suppression * SuppressionFOVPenalty;
    FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(
        FirstPersonCamera->FieldOfView,
        TargetFOV,
        DeltaSeconds,
        AimInterpSpeed));

    const float SuppressionRoll =
        FMath::Sin(Time * 7.5f) *
        Suppression *
        SuppressionRollDegrees;

    FirstPersonCamera->SetRelativeRotation(FMath::RInterpTo(
        FirstPersonCamera->GetRelativeRotation(),
        FRotator(0.0f, 0.0f, SuppressionRoll),
        DeltaSeconds,
        9.0f));

    const float AimSwayScale = bAiming ? 0.38f : 1.0f;
    const float StanceSwayScale = bProne
        ? 0.48f
        : (bIsCrouched ? 0.72f : 1.0f);

    const float SwayAmplitude = BaseWeaponSwayCm *
        AimSwayScale *
        StanceSwayScale *
        (1.0f + SpeedAlpha * 1.8f + Suppression * 1.25f);

    const FVector CameraTarget(
        -8.0f,
        0.0f,
        bProne ? ProneCameraZ : StandingCameraZ);

    FirstPersonCamera->SetRelativeLocation(FMath::VInterpTo(
        FirstPersonCamera->GetRelativeLocation(),
        CameraTarget,
        DeltaSeconds,
        StanceInterpSpeed));

    const float SwayPhase = Time * FMath::Lerp(1.6f, 3.1f, SpeedAlpha);
    const FVector SwayOffset(
        0.0f,
        FMath::Sin(SwayPhase) * SwayAmplitude,
        FMath::Cos(SwayPhase * 0.82f) * SwayAmplitude * 0.55f);

    const FVector BodyTarget =
        (bAiming ? AimBodyLocation : HipBodyLocation) + SwayOffset;

    const FVector BarrelTarget =
        (bAiming ? AimBarrelLocation : HipBarrelLocation) + SwayOffset * 0.72f;

    ViewWeaponBody->SetRelativeLocation(FMath::VInterpTo(
        ViewWeaponBody->GetRelativeLocation(),
        BodyTarget,
        DeltaSeconds,
        AimInterpSpeed));

    ViewWeaponBarrel->SetRelativeLocation(FMath::VInterpTo(
        ViewWeaponBarrel->GetRelativeLocation(),
        BarrelTarget,
        DeltaSeconds,
        AimInterpSpeed));
}

void AWPSoldierCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AWPSoldierCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AWPSoldierCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AWPSoldierCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AWPSoldierCharacter::LookUp);

    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AWPSoldierCharacter::JumpInput);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("Prone"), IE_Pressed, this, &AWPSoldierCharacter::ToggleProne);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AWPSoldierCharacter::StartFireInput);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &AWPSoldierCharacter::StopFireInput);
    PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &AWPSoldierCharacter::ReloadInput);
    PlayerInputComponent->BindAction(TEXT("Melee"), IE_Pressed, this, &AWPSoldierCharacter::MeleeInput);
    PlayerInputComponent->BindAction(TEXT("Bandage"), IE_Pressed, this, &AWPSoldierCharacter::StartBandage);
    PlayerInputComponent->BindAction(TEXT("SwitchSoldier"), IE_Pressed, this, &AWPSoldierCharacter::SwitchSquadmateInput);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AWPSoldierCharacter::StartSprint);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AWPSoldierCharacter::StopSprint);
    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &AWPSoldierCharacter::StartCrouch);
    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Released, this, &AWPSoldierCharacter::StopCrouch);
    PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &AWPSoldierCharacter::StartAim);
    PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &AWPSoldierCharacter::StopAim);
    PlayerInputComponent->BindAction(TEXT("ThrowGrenade"), IE_Pressed, this, &AWPSoldierCharacter::ThrowGrenadeInput);
    PlayerInputComponent->BindAction(TEXT("ThrowSmoke"), IE_Pressed, this, &AWPSoldierCharacter::ThrowSmokeGrenadeInput);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AWPSoldierCharacter::InteractInput);
    PlayerInputComponent->BindAction(TEXT("SquadFollow"), IE_Pressed, this, &AWPSoldierCharacter::IssueFollowOrder);
    PlayerInputComponent->BindAction(TEXT("SquadAttack"), IE_Pressed, this, &AWPSoldierCharacter::IssueAttackOrder);
    PlayerInputComponent->BindAction(TEXT("SquadHold"), IE_Pressed, this, &AWPSoldierCharacter::IssueHoldOrder);
    PlayerInputComponent->BindAction(TEXT("CallArtillery"), IE_Pressed, this, &AWPSoldierCharacter::CallArtilleryInput);
}

void AWPSoldierCharacter::StartFireInput()
{
    if (!IsCombatCapable())
    {
        return;
    }

    CancelBandage();
    WeaponComponent->StartFire();
}

void AWPSoldierCharacter::StopFireInput()
{
    WeaponComponent->StopFire();
}

void AWPSoldierCharacter::ReloadInput()
{
    if (!IsCombatCapable())
    {
        return;
    }

    CancelBandage();
    WeaponComponent->Reload();
}

void AWPSoldierCharacter::MeleeInput()
{
    if (!GetWorld() || !IsCombatCapable())
    {
        return;
    }

    AWPSoldierCharacter* BestTarget = nullptr;
    float BestScore = TNumericLimits<float>::Max();

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Candidate = *It;
        if (!Candidate ||
            Candidate == this ||
            Candidate->GetCombatTeam() == CombatTeam ||
            !Candidate->GetHealthComponent() ||
            Candidate->GetHealthComponent()->IsDead())
        {
            continue;
        }

        const FVector ToCandidate =
            Candidate->GetActorLocation() - GetActorLocation();
        const float Distance = ToCandidate.Size();

        if (Distance > MeleeRange)
        {
            continue;
        }

        const float Facing = FVector::DotProduct(
            GetActorForwardVector(),
            ToCandidate.GetSafeNormal());

        if (Facing < 0.35f)
        {
            continue;
        }

        const float Score = Distance - Facing * 80.0f;
        if (Score < BestScore)
        {
            BestScore = Score;
            BestTarget = Candidate;
        }
    }

    if (BestTarget)
    {
        TryMeleeAttack(BestTarget);
    }
}

void AWPSoldierCharacter::StartBandage()
{
    if (!GetWorld() ||
        !IsCombatCapable() ||
        bBandaging ||
        MedkitsRemaining <= 0 ||
        !HealthComponent ||
        HealthComponent->GetHealth() >= HealthComponent->GetMaxHealth())
    {
        return;
    }

    StopSprint();
    StopAim();
    WeaponComponent->StopFire();
    WeaponComponent->CancelReload();

    bBandaging = true;
    OnBandageStarted.Broadcast();
    GetCharacterMovement()->MaxWalkSpeed =
        bProne ? ProneSpeed : FMath::Min(WalkSpeed, 220.0f);

    GetWorld()->GetTimerManager().SetTimer(
        BandageTimerHandle,
        this,
        &AWPSoldierCharacter::FinishBandage,
        BandageDurationSeconds,
        false);
}

void AWPSoldierCharacter::FinishBandage()
{
    if (!bBandaging)
    {
        return;
    }

    bBandaging = false;

    if (HealthComponent &&
        HealthComponent->Heal(BandageHealAmount))
    {
        MedkitsRemaining = FMath::Max(0, MedkitsRemaining - 1);
    }

    GetCharacterMovement()->MaxWalkSpeed =
        bProne ? ProneSpeed : WalkSpeed;

    OnBandageCompleted.Broadcast();
}

void AWPSoldierCharacter::CancelBandage()
{
    if (!bBandaging)
    {
        return;
    }

    bBandaging = false;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(BandageTimerHandle);
    }

    GetCharacterMovement()->MaxWalkSpeed =
        bProne ? ProneSpeed : WalkSpeed;
}

void AWPSoldierCharacter::MoveForward(float Value)
{
    if (IsCombatCapable() && !FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

void AWPSoldierCharacter::MoveRight(float Value)
{
    if (IsCombatCapable() && !FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void AWPSoldierCharacter::Turn(float Value)
{
    if (!HealthComponent->IsDead())
    {
        AddControllerYawInput(Value);
    }
}

void AWPSoldierCharacter::LookUp(float Value)
{
    if (!HealthComponent->IsDead())
    {
        AddControllerPitchInput(Value);
    }
}

void AWPSoldierCharacter::StartSprint()
{
    if (!IsCombatCapable() || Stamina <= 8.0f)
    {
        return;
    }

    CancelBandage();

    if (bProne && !ExitProne())
    {
        return;
    }

    StopAim();
    UnCrouch();
    bSprinting = true;
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AWPSoldierCharacter::StopSprint()
{
    bSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

    if (GetWorld())
    {
        LastSprintActivityTime = GetWorld()->GetTimeSeconds();
    }
}

void AWPSoldierCharacter::JumpInput()
{
    if (!IsCombatCapable())
    {
        return;
    }

    if (bProne)
    {
        ExitProne();
        return;
    }

    if (bIsCrouched)
    {
        UnCrouch();
        return;
    }

    Jump();
}

void AWPSoldierCharacter::ToggleProne()
{
    if (!IsCombatCapable())
    {
        return;
    }

    if (bProne)
    {
        ExitProne();
    }
    else
    {
        EnterProne();
    }
}

bool AWPSoldierCharacter::EnterProne()
{
    if (bProne || !IsCombatCapable())
    {
        return bProne;
    }

    StopSprint();
    UnCrouch();

    UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (!Capsule)
    {
        return false;
    }

    const float CurrentHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
    const float HeightDelta = FMath::Max(
        0.0f,
        CurrentHalfHeight - ProneCapsuleHalfHeight);

    Capsule->SetCapsuleHalfHeight(ProneCapsuleHalfHeight, true);
    AddActorWorldOffset(
        FVector(0.0f, 0.0f, -HeightDelta),
        true);

    bProne = true;
    GetCharacterMovement()->MaxWalkSpeed = ProneSpeed;
    return true;
}

bool AWPSoldierCharacter::ExitProne()
{
    if (!bProne)
    {
        return true;
    }

    UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (!Capsule || !GetWorld())
    {
        return false;
    }

    const float HeightDelta = FMath::Max(
        0.0f,
        StandingCapsuleHalfHeight - ProneCapsuleHalfHeight);

    const FVector DesiredCenter =
        GetActorLocation() + FVector(0.0f, 0.0f, HeightDelta);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WPExitProne), false, this);
    QueryParams.AddIgnoredActor(this);

    const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
        DesiredCenter,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeCapsule(
            Capsule->GetUnscaledCapsuleRadius(),
            StandingCapsuleHalfHeight),
        QueryParams);

    if (bBlocked)
    {
        return false;
    }

    AddActorWorldOffset(
        FVector(0.0f, 0.0f, HeightDelta),
        false);

    Capsule->SetCapsuleHalfHeight(StandingCapsuleHalfHeight, true);
    bProne = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    return true;
}

void AWPSoldierCharacter::StartCrouch()
{
    if (!IsCombatCapable())
    {
        return;
    }

    if (bProne)
    {
        ExitProne();
        return;
    }

    Crouch();
}

void AWPSoldierCharacter::StopCrouch()
{
    if (!bProne)
    {
        UnCrouch();
    }
}

void AWPSoldierCharacter::StartAim()
{
    if (!IsPlayerControlled() || !IsCombatCapable() || WeaponComponent->IsReloading())
    {
        return;
    }

    CancelBandage();
    bAiming = true;
    WeaponComponent->SetAiming(true);
}

void AWPSoldierCharacter::StopAim()
{
    bAiming = false;
    WeaponComponent->SetAiming(false);
}

void AWPSoldierCharacter::ThrowGrenadeInput()
{
    CancelBandage();
    WeaponComponent->CancelReload();
    FVector TargetLocation;
    if (GetViewTarget(TargetLocation, 6500.0f))
    {
        ThrowGrenadeAt(TargetLocation);
    }
}

void AWPSoldierCharacter::ThrowSmokeGrenadeInput()
{
    CancelBandage();
    WeaponComponent->CancelReload();
    FVector TargetLocation;
    if (GetViewTarget(TargetLocation, 6000.0f))
    {
        ThrowSmokeGrenadeAt(TargetLocation);
    }
}

void AWPSoldierCharacter::InteractInput()
{
    CancelBandage();

    if (!GetWorld() || !IsCombatCapable())
    {
        return;
    }

    AWPSoldierCharacter* BestTarget = nullptr;
    float BestDistanceSquared = FMath::Square(ReviveInteractionRange);

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Candidate = *It;
        if (!Candidate ||
            Candidate == this ||
            Candidate->GetCombatTeam() != CombatTeam ||
            !Candidate->IsDowned())
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared(
            GetActorLocation(),
            Candidate->GetActorLocation());

        if (DistanceSquared <= BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestTarget = Candidate;
        }
    }

    if (BestTarget)
    {
        TryReviveSoldier(BestTarget);
    }
}

void AWPSoldierCharacter::SwitchSquadmateInput()
{
    if (AWPBattlefieldDirector* Director = GetBattlefieldDirector())
    {
        Director->SwitchPlayerToSquadmate(this);
    }
}

void AWPSoldierCharacter::IssueFollowOrder()
{
    if (AWPBattlefieldDirector* Director = GetBattlefieldDirector())
    {
        Director->IssueSquadOrder(EWPSquadOrder::Follow, GetActorLocation());
    }
}

void AWPSoldierCharacter::IssueAttackOrder()
{
    if (AWPBattlefieldDirector* Director = GetBattlefieldDirector())
    {
        Director->IssueSquadOrder(EWPSquadOrder::AttackObjective, Director->GetActiveObjectiveLocation());
    }
}

void AWPSoldierCharacter::IssueHoldOrder()
{
    if (AWPBattlefieldDirector* Director = GetBattlefieldDirector())
    {
        Director->IssueSquadOrder(EWPSquadOrder::HoldPosition, GetActorLocation());
    }
}

void AWPSoldierCharacter::CallArtilleryInput()
{
    FVector TargetLocation;
    if (GetViewTarget(TargetLocation, 9000.0f))
    {
        if (AWPBattlefieldDirector* Director = GetBattlefieldDirector())
        {
            Director->RequestArtillery(this, TargetLocation);
        }
    }
}

void AWPSoldierCharacter::HandleShotFired(float RecoilPitch, float RecoilYaw)
{
    if (IsCombatCapable())
    {
        if (AWPBattlefieldDirector* Director = GetBattlefieldDirector())
        {
            Director->ReportGunshot(
                CombatTeam,
                GetActorLocation(),
                9000.0f);
        }
    }

    if (!IsPlayerControlled() || !IsCombatCapable())
    {
        return;
    }

    AddControllerPitchInput(-RecoilPitch);
    AddControllerYawInput(RecoilYaw);

    PendingRecoilPitch += RecoilPitch * 0.72f;
    PendingRecoilYaw += RecoilYaw * 0.58f;

    FirstPersonCamera->SetFieldOfView(
        FirstPersonCamera->FieldOfView +
        RecoilCameraImpulseFOV * (bAiming ? 0.55f : 1.0f));
}

void AWPSoldierCharacter::HandleDowned()
{
    CancelBandage();
    StopSprint();
    ExitProne();
    StopAim();
    WeaponComponent->StopFire();
    WeaponComponent->CancelReload();
    GetCharacterMovement()->DisableMovement();
}

void AWPSoldierCharacter::HandleRevived()
{
    bSprinting = false;
    Stamina = FMath::Max(Stamina, MaxStamina * 0.5f);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    PendingRecoilPitch = 0.0f;
    PendingRecoilYaw = 0.0f;
    SuppressionComponent->ClearSuppression();
}

void AWPSoldierCharacter::HandleDeath()
{
    CancelBandage();
    ExitProne();
    StopAim();
    WeaponComponent->StopFire();
    GetCharacterMovement()->DisableMovement();

    if (AWPBattlefieldDirector* Director = GetBattlefieldDirector())
    {
        Director->NotifySoldierKilled(this);
    }

    if (!IsPlayerControlled())
    {
        SetLifeSpan(3.0f);
    }
}
