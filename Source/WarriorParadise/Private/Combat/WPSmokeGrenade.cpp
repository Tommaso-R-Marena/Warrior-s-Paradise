#include "Combat/WPSmokeGrenade.h"
#include "Combat/WPSmokeCloud.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"

AWPSmokeGrenade::AWPSmokeGrenade()
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
    Movement->Bounciness = 0.24f;
    Movement->Friction = 0.8f;
    Movement->ProjectileGravityScale = 1.0f;
    Movement->InitialSpeed = 1350.0f;
    Movement->MaxSpeed = 2000.0f;

    SmokeCloudClass = AWPSmokeCloud::StaticClass();
}

void AWPSmokeGrenade::BeginPlay()
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
            &AWPSmokeGrenade::DeploySmoke,
            FuseSeconds,
            false);
    }
}

void AWPSmokeGrenade::InitializeSmokeGrenade(const FVector& LaunchVelocity)
{
    Movement->Velocity = LaunchVelocity;
}

void AWPSmokeGrenade::DeploySmoke()
{
    if (GetWorld() && SmokeCloudClass)
    {
        if (AWPSmokeCloud* Smoke = GetWorld()->SpawnActor<AWPSmokeCloud>(
            SmokeCloudClass,
            GetActorLocation(),
            FRotator::ZeroRotator))
        {
            Smoke->InitializeSmoke(SmokeRadiusCm, SmokeDurationSeconds);
        }
    }

    Destroy();
}
