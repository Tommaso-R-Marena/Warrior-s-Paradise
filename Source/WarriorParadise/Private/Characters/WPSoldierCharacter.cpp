#include "Characters/WPSoldierCharacter.h"
#include "Camera/CameraComponent.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AWPSoldierCharacter::AWPSoldierCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

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
    FirstPersonCamera->bUsePawnControlRotation = true;

    HealthComponent = CreateDefaultSubobject<UWPHealthComponent>(TEXT("HealthComponent"));
    WeaponComponent = CreateDefaultSubobject<UWPWeaponComponent>(TEXT("WeaponComponent"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    ViewWeaponBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ViewWeaponBody"));
    ViewWeaponBody->SetupAttachment(FirstPersonCamera);
    ViewWeaponBody->SetOnlyOwnerSee(true);
    ViewWeaponBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ViewWeaponBody->CastShadow = false;
    ViewWeaponBody->SetRelativeLocation(FVector(45.0f, 18.0f, -18.0f));
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
    ViewWeaponBarrel->SetRelativeLocation(FVector(82.0f, 18.0f, -15.0f));
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
    HealthComponent->OnDeath.AddDynamic(this, &AWPSoldierCharacter::HandleDeath);
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

void AWPSoldierCharacter::HandleDeath()
{
    WeaponComponent->StopFire();
    GetCharacterMovement()->DisableMovement();

    if (!IsPlayerControlled())
    {
        SetLifeSpan(2.0f);
    }
}
