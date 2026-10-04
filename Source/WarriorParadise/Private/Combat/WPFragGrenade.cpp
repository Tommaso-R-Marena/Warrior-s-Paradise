#include "Combat/WPFragGrenade.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSuppressionComponent.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Vehicles/WPArmoredVehicle.h"
#include "World/WPDestructibleCover.h"

AWPFragGrenade::AWPFragGrenade()
{
    PrimaryActorTick.bCanEverTick = false;
    InitialLifeSpan = 8.0f;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(6.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Block);

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->bRotationFollowsVelocity = true;
    Movement->bShouldBounce = true;
    Movement->Bounciness = 0.28f;
    Movement->Friction = 0.72f;
    Movement->ProjectileGravityScale = 1.0f;
    Movement->InitialSpeed = 1500.0f;
    Movement->MaxSpeed = 2200.0f;
}

void AWPFragGrenade::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* GrenadeOwner = GetOwner())
    {
        Collision->IgnoreActorWhenMoving(GrenadeOwner, true);
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            FuseTimerHandle,
            this,
            &AWPFragGrenade::Explode,
            FuseSeconds,
            false);
    }
}

void AWPFragGrenade::InitializeGrenade(EWPCombatTeam InTeam, const FVector& LaunchVelocity)
{
    GrenadeTeam = InTeam;
    Movement->Velocity = LaunchVelocity;
}

void AWPFragGrenade::Explode()
{
    if (!GetWorld())
    {
        Destroy();
        return;
    }

    const FVector ExplosionLocation = GetActorLocation();

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Soldier = *It;
        if (!Soldier ||
            Soldier->GetCombatTeam() == GrenadeTeam ||
            !Soldier->GetHealthComponent() ||
            Soldier->GetHealthComponent()->IsDead())
        {
            continue;
        }

        const float Distance = FVector::Dist(ExplosionLocation, Soldier->GetActorLocation());
        if (Distance <= SuppressionRadiusCm && Soldier->GetSuppressionComponent())
        {
            const float SuppressionAlpha = 1.0f - FMath::Clamp(Distance / SuppressionRadiusCm, 0.0f, 1.0f);
            Soldier->GetSuppressionComponent()->AddSuppression(65.0f * SuppressionAlpha);
        }

        if (Distance > BlastRadiusCm)
        {
            continue;
        }

        FHitResult VisibilityHit;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WPGrenadeOcclusion), true, this);
        QueryParams.AddIgnoredActor(this);

        const FVector TargetLocation = Soldier->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
        const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
            VisibilityHit,
            ExplosionLocation,
            TargetLocation,
            ECC_Visibility,
            QueryParams) &&
            VisibilityHit.GetActor() != Soldier;

        const float DistanceAlpha = 1.0f - FMath::Clamp(Distance / BlastRadiusCm, 0.0f, 1.0f);
        const float OcclusionScale = bBlocked ? 0.28f : 1.0f;
        const float Damage = MaximumDamage * FMath::Square(DistanceAlpha) * OcclusionScale;

        if (Damage > 0.5f)
        {
            UGameplayStatics::ApplyDamage(
                Soldier,
                Damage,
                GetInstigatorController(),
                this,
                UDamageType::StaticClass());
        }
    }

    for (TActorIterator<AWPArmoredVehicle> It(GetWorld()); It; ++It)
    {
        AWPArmoredVehicle* Vehicle = *It;
        if (!Vehicle ||
            Vehicle->GetCombatTeam() == GrenadeTeam ||
            Vehicle->IsDestroyed())
        {
            continue;
        }

        const float Distance = FVector::Dist(ExplosionLocation, Vehicle->GetActorLocation());
        if (Distance <= BlastRadiusCm)
        {
            const float Alpha = 1.0f - FMath::Clamp(Distance / BlastRadiusCm, 0.0f, 1.0f);
            UGameplayStatics::ApplyDamage(
                Vehicle,
                MaximumDamage * 0.55f * FMath::Square(Alpha),
                GetInstigatorController(),
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

        const float Distance = FVector::Dist(ExplosionLocation, Cover->GetActorLocation());
        if (Distance > BlastRadiusCm)
        {
            continue;
        }

        const float DistanceAlpha = 1.0f - FMath::Clamp(Distance / BlastRadiusCm, 0.0f, 1.0f);
        UGameplayStatics::ApplyDamage(
            Cover,
            MaximumDamage * 0.9f * FMath::Square(DistanceAlpha),
            GetInstigatorController(),
            this,
            UDamageType::StaticClass());
    }

    OnExploded.Broadcast(ExplosionLocation);
    Destroy();
}
