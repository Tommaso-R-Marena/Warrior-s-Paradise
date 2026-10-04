#include "Combat/WPBallisticProjectile.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPSuppressionComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AWPBallisticProjectile::AWPBallisticProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    InitialLifeSpan = 5.0f;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(2.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Block);
    Collision->SetNotifyRigidBodyCollision(true);
    Collision->OnComponentHit.AddDynamic(this, &AWPBallisticProjectile::HandleHit);

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->bRotationFollowsVelocity = true;
    Movement->bShouldBounce = false;
    Movement->ProjectileGravityScale = 1.0f;
    Movement->InitialSpeed = 80000.0f;
    Movement->MaxSpeed = 90000.0f;
}

void AWPBallisticProjectile::BeginPlay()
{
    Super::BeginPlay();
    SpawnLocation = GetActorLocation();

    if (AActor* ProjectileOwner = GetOwner())
    {
        Collision->IgnoreActorWhenMoving(ProjectileOwner, true);
    }
}

void AWPBallisticProjectile::InitializeProjectile(
    const FVector& Direction,
    float MuzzleVelocityCmPerSecond,
    float InDamage,
    float InMaximumTravelCm,
    AController* InInstigatorController,
    EWPCombatTeam InTeam)
{
    const float Speed = FMath::Max(1000.0f, MuzzleVelocityCmPerSecond);
    Damage = FMath::Max(0.0f, InDamage);
    MaximumTravelCm = FMath::Max(1000.0f, InMaximumTravelCm);
    InstigatorController = InInstigatorController;
    ProjectileTeam = InTeam;

    Movement->InitialSpeed = Speed;
    Movement->MaxSpeed = Speed;
    Movement->Velocity = Direction.GetSafeNormal() * Speed;
}

void AWPBallisticProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ApplyNearMissSuppression();

    if (FVector::DistSquared(SpawnLocation, GetActorLocation()) > FMath::Square(MaximumTravelCm))
    {
        Destroy();
    }
}

void AWPBallisticProjectile::ApplyNearMissSuppression()
{
    if (!GetWorld() || NearMissRadiusCm <= 0.0f || NearMissSuppression <= 0.0f)
    {
        return;
    }

    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams ObjectQuery;
    ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WPNearMissSuppression), false, this);
    if (AActor* ProjectileOwner = GetOwner())
    {
        QueryParams.AddIgnoredActor(ProjectileOwner);
    }

    if (!GetWorld()->OverlapMultiByObjectType(
        Overlaps,
        GetActorLocation(),
        FQuat::Identity,
        ObjectQuery,
        FCollisionShape::MakeSphere(NearMissRadiusCm),
        QueryParams))
    {
        return;
    }

    for (const FOverlapResult& Result : Overlaps)
    {
        AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(Result.GetActor());
        if (!Soldier ||
            Soldier == GetOwner() ||
            Soldier->GetCombatTeam() == ProjectileTeam ||
            !Soldier->GetSuppressionComponent())
        {
            continue;
        }

        TWeakObjectPtr<AActor> WeakSoldier(Soldier);
        if (SuppressedActors.Contains(WeakSoldier))
        {
            continue;
        }

        Soldier->GetSuppressionComponent()->AddSuppression(NearMissSuppression);
        SuppressedActors.Add(WeakSoldier);
    }
}

void AWPBallisticProjectile::HandleHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == GetOwner())
    {
        return;
    }

    if (AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(OtherActor))
    {
        if (Soldier->GetCombatTeam() == ProjectileTeam)
        {
            Destroy();
            return;
        }

        if (UWPSuppressionComponent* Suppression = Soldier->GetSuppressionComponent())
        {
            Suppression->AddSuppression(30.0f);
        }
    }

    const FVector ShotDirection = Movement->Velocity.GetSafeNormal();
    UGameplayStatics::ApplyPointDamage(
        OtherActor,
        Damage,
        ShotDirection,
        Hit,
        InstigatorController.Get(),
        GetOwner(),
        UDamageType::StaticClass());

    Destroy();
}
