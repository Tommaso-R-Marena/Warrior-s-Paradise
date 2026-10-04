#include "Vehicles/WPAirBomb.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSuppressionComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/WPExplosionPresentationProfile.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/WPArmoredVehicle.h"
#include "Vehicles/WPLightVehicle.h"
#include "World/WPDestructibleCover.h"
#include "World/WPGunEmplacement.h"

AWPAirBomb::AWPAirBomb()
{
    PrimaryActorTick.bCanEverTick = false;
    InitialLifeSpan = 12.0f;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(12.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Block);
    Collision->SetNotifyRigidBodyCollision(true);
    Collision->OnComponentHit.AddDynamic(this, &AWPAirBomb::HandleHit);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Collision);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.5f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderAsset.Succeeded())
    {
        Visual->SetStaticMesh(CylinderAsset.Object);
    }

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->bRotationFollowsVelocity = true;
    Movement->bShouldBounce = false;
    Movement->ProjectileGravityScale = 1.05f;
    Movement->InitialSpeed = 1200.0f;
    Movement->MaxSpeed = 6000.0f;
}

void AWPAirBomb::InitializeBomb(
    EWPCombatTeam InTeam,
    const FVector& InitialVelocity,
    UWPExplosionPresentationProfile* InPresentationProfile)
{
    BombTeam = InTeam;
    PresentationProfile = InPresentationProfile;
    Movement->Velocity = InitialVelocity;
    Movement->InitialSpeed = InitialVelocity.Size();
}

void AWPAirBomb::HandleHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (OtherActor == GetOwner())
    {
        return;
    }

    Explode(Hit.ImpactPoint);
}

void AWPAirBomb::Explode(const FVector& ImpactLocation)
{
    if (bExploded || !GetWorld())
    {
        return;
    }

    bExploded = true;

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Soldier = *It;
        if (!Soldier ||
            Soldier->GetCombatTeam() == BombTeam ||
            !Soldier->GetHealthComponent() ||
            Soldier->GetHealthComponent()->IsDead())
        {
            continue;
        }

        const float Distance = FVector::Dist(
            ImpactLocation,
            Soldier->GetActorLocation());

        if (Distance <= SuppressionRadiusCm &&
            Soldier->GetSuppressionComponent())
        {
            const float Alpha = 1.0f -
                FMath::Clamp(
                    Distance / SuppressionRadiusCm,
                    0.0f,
                    1.0f);
            Soldier->GetSuppressionComponent()->AddSuppression(
                100.0f * Alpha);
        }

        if (Distance <= BlastRadiusCm)
        {
            const float Alpha = 1.0f -
                FMath::Clamp(
                    Distance / BlastRadiusCm,
                    0.0f,
                    1.0f);

            UGameplayStatics::ApplyDamage(
                Soldier,
                MaximumInfantryDamage * FMath::Square(Alpha),
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    for (TActorIterator<AWPArmoredVehicle> It(GetWorld()); It; ++It)
    {
        AWPArmoredVehicle* Vehicle = *It;
        if (!Vehicle ||
            Vehicle->GetCombatTeam() == BombTeam ||
            Vehicle->IsDestroyed())
        {
            continue;
        }

        const float Distance = FVector::Dist(
            ImpactLocation,
            Vehicle->GetActorLocation());

        if (Distance <= BlastRadiusCm)
        {
            const float Alpha = 1.0f -
                FMath::Clamp(
                    Distance / BlastRadiusCm,
                    0.0f,
                    1.0f);

            UGameplayStatics::ApplyDamage(
                Vehicle,
                VehicleDamage * FMath::Lerp(0.35f, 1.0f, Alpha),
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    for (TActorIterator<AWPLightVehicle> It(GetWorld()); It; ++It)
    {
        AWPLightVehicle* Vehicle = *It;
        if (!Vehicle ||
            Vehicle->GetCombatTeam() == BombTeam ||
            Vehicle->IsDestroyed())
        {
            continue;
        }

        const float Distance = FVector::Dist(
            ImpactLocation,
            Vehicle->GetActorLocation());

        if (Distance <= BlastRadiusCm)
        {
            const float Alpha = 1.0f -
                FMath::Clamp(
                    Distance / BlastRadiusCm,
                    0.0f,
                    1.0f);

            UGameplayStatics::ApplyDamage(
                Vehicle,
                VehicleDamage * FMath::Lerp(0.45f, 1.1f, Alpha),
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    for (TActorIterator<AWPGunEmplacement> It(GetWorld()); It; ++It)
    {
        AWPGunEmplacement* Gun = *It;
        if (!Gun ||
            Gun->GetCombatTeam() == BombTeam ||
            Gun->IsDestroyed())
        {
            continue;
        }

        const float Distance = FVector::Dist(
            ImpactLocation,
            Gun->GetActorLocation());

        if (Distance <= BlastRadiusCm)
        {
            UGameplayStatics::ApplyDamage(
                Gun,
                VehicleDamage,
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    for (TActorIterator<AWPDestructibleCover> It(GetWorld()); It; ++It)
    {
        AWPDestructibleCover* Cover = *It;
        if (!Cover)
        {
            continue;
        }

        const float Distance = FVector::Dist(
            ImpactLocation,
            Cover->GetActorLocation());

        if (Distance <= BlastRadiusCm)
        {
            UGameplayStatics::ApplyDamage(
                Cover,
                MaximumInfantryDamage * 0.8f,
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    if (PresentationProfile)
    {
        if (UNiagaraSystem* System =
            PresentationProfile->ExplosionEffect.LoadSynchronous())
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                this,
                System,
                ImpactLocation,
                FRotator::ZeroRotator,
                PresentationProfile->EffectScale);
        }

        if (USoundBase* Sound =
            PresentationProfile->ExplosionSound.LoadSynchronous())
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,
                Sound,
                ImpactLocation,
                PresentationProfile->Volume,
                FMath::FRandRange(0.97f, 1.03f),
                0.0f,
                PresentationProfile->Attenuation.LoadSynchronous(),
                nullptr,
                nullptr);
        }
    }

    Destroy();
}
