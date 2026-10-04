#include "Characters/WPSoldierCharacter.h"
#include "Camera/CameraComponent.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSuppressionComponent.h"
#include "Combat/WPWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/WPBattlefieldDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AWPSoldierCharacter::AWPSoldierCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 94.0f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 255.0f;
    GetCharacterMovement()->AirControl = 0.24f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1750.0f;

    bUseControllerRotationYaw = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(-8.0f, 0.0f, 68.0f));
    FirstPersonCamera->SetFieldOfView(HipFOV);
    FirstPersonCamera->bUsePawnControlRotation = true;

    HealthComponent = CreateDefaultSubobject<UWPHealthComponent>(TEXT("HealthComponent"));
    WeaponComponent = CreateDefaultSubobject<UWPWeaponComponent>(TEXT("WeaponComponent"));
    SuppressionComponent = CreateDefaultSubobject<UWPSuppressionComponent>(TEXT("SuppressionComponent"));

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

    HealthComponent->OnDeath.AddDynamic(this, &AWPSoldierCharacter::HandleDeath);
    WeaponComponent->OnShotFired.AddDynamic(this, &AWPSoldierCharacter::HandleShotFired);
}

void AWPSoldierCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!IsPlayerControlled())
    {
        return;
    }

    const float TargetFOV = bAiming ? AimFOV : HipFOV;
    FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(
        FirstPersonCamera->FieldOfView,
        TargetFOV,
        DeltaSeconds,
        AimInterpSpeed));

    const FVector BodyTarget = bAiming ? AimBodyLocation : HipBodyLocation;
    const FVector BarrelTarget = bAiming ? AimBarrelLocation : HipBarrelLocation;

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

    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, WeaponComponent, &UWPWeaponComponent::StartFire);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, WeaponComponent, &UWPWeaponComponent::StopFire);
    PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, WeaponComponent, &UWPWeaponComponent::Reload);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AWPSoldierCharacter::StartSprint);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AWPSoldierCharacter::StopSprint);
    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &AWPSoldierCharacter::StartCrouch);
    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Released, this, &AWPSoldierCharacter::StopCrouch);
    PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &AWPSoldierCharacter::StartAim);
    PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &AWPSoldierCharacter::StopAim);
}

void AWPSoldierCharacter::MoveForward(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

void AWPSoldierCharacter::MoveRight(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void AWPSoldierCharacter::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void AWPSoldierCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

void AWPSoldierCharacter::StartSprint()
{
    StopAim();
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AWPSoldierCharacter::StopSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AWPSoldierCharacter::StartCrouch()
{
    Crouch();
}

void AWPSoldierCharacter::StopCrouch()
{
    UnCrouch();
}

void AWPSoldierCharacter::StartAim()
{
    if (!IsPlayerControlled() || HealthComponent->IsDead() || WeaponComponent->IsReloading())
    {
        return;
    }

    bAiming = true;
    WeaponComponent->SetAiming(true);
}

void AWPSoldierCharacter::StopAim()
{
    bAiming = false;
    WeaponComponent->SetAiming(false);
}

void AWPSoldierCharacter::HandleShotFired(float RecoilPitch, float RecoilYaw)
{
    if (!IsPlayerControlled() || HealthComponent->IsDead())
    {
        return;
    }

    AddControllerPitchInput(-RecoilPitch);
    AddControllerYawInput(RecoilYaw);
}

void AWPSoldierCharacter::HandleDeath()
{
    StopAim();
    WeaponComponent->StopFire();
    GetCharacterMovement()->DisableMovement();

    if (AWPBattlefieldDirector* Director = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass())))
    {
        Director->NotifySoldierKilled(this);
    }

    if (!IsPlayerControlled())
    {
        SetLifeSpan(2.0f);
    }
}
