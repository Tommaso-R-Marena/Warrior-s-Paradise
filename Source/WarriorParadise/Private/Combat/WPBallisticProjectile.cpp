#include "Combat/WPBallisticProjectile.h"
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
    AController* InInstigatorController)
{
    const float Speed = FMath::Max(1000.0f, MuzzleVelocityCmPerSecond);
    Damage = FMath::Max(0.0f, InDamage);
    MaximumTravelCm = FMath::Max(1000.0f, InMaximumTravelCm);
    InstigatorController = InInstigatorController;

    Movement->InitialSpeed = Speed;
    Movement->MaxSpeed = Speed;
    Movement->Velocity = Direction.GetSafeNormal() * Speed;
}

void AWPBallisticProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (FVector::DistSquared(SpawnLocation, GetActorLocation()) > FMath::Square(MaximumTravelCm))
    {
        Destroy();
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
