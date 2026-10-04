#include "AI/WPEnemySoldier.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/WPBattlefieldDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AWPEnemySoldier::AWPEnemySoldier()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    GetCharacterMovement()->MaxWalkSpeed = 315.0f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    TorsoVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TorsoVisual"));
    TorsoVisual->SetupAttachment(GetCapsuleComponent());
    TorsoVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TorsoVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
    TorsoVisual->SetRelativeScale3D(FVector(0.34f, 0.22f, 0.52f));
    if (CubeAsset.Succeeded())
    {
        TorsoVisual->SetStaticMesh(CubeAsset.Object);
    }

    HeadVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadVisual"));
    HeadVisual->SetupAttachment(GetCapsuleComponent());
    HeadVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeadVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 76.0f));
    HeadVisual->SetRelativeScale3D(FVector(0.21f));
    if (SphereAsset.Succeeded())
    {
        HeadVisual->SetStaticMesh(SphereAsset.Object);
    }

    RifleVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RifleVisual"));
    RifleVisual->SetupAttachment(GetCapsuleComponent());
    RifleVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

    GetWeaponComponent()->Configure(9.0f, 360.0f, 4.2f);
    GetHealthComponent()->OnDeath.AddDynamic(this, &AWPEnemySoldier::HandleEnemyDeath);
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

    if (GetHealthComponent()->IsDead() || !GetWorld())
    {
        return;
    }

    ACharacter* Target = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (!Target)
    {
        return;
    }

    const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    const float Distance = ToTarget.Size();
    const FVector FlatDirection = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();

    if (!FlatDirection.IsNearlyZero())
    {
        const FRotator DesiredRotation = FlatDirection.Rotation();
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, AimTurnSpeed));
    }

    if (Distance > DesiredRange)
    {
        AddMovementInput(FlatDirection, 0.75f);
    }
    else if (Distance < DesiredRange * 0.62f)
    {
        AddMovementInput(-FlatDirection, 0.35f);
    }

    const float Time = GetWorld()->GetTimeSeconds();

    if (bPulseFiring && Time >= StopFireTime)
    {
        GetWeaponComponent()->StopFire();
        bPulseFiring = false;
    }

    if (Time >= NextDecisionTime)
    {
        NextDecisionTime = Time + FMath::FRandRange(0.35f, 0.75f);

        if (Distance <= MaximumEngagementRange && HasLineOfSightTo(Target))
        {
            GetWeaponComponent()->StartFire();
            bPulseFiring = true;
            StopFireTime = Time + FirePulseSeconds;
        }
    }
}

void AWPEnemySoldier::HandleEnemyDeath()
{
    GetWeaponComponent()->StopFire();

    if (AWPBattlefieldDirector* Director = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass())))
    {
        Director->NotifyEnemyKilled(this);
    }
}
