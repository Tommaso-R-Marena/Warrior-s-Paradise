#include "World/WPDestructibleCover.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AWPDestructibleCover::AWPDestructibleCover()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoverMesh"));
    SetRootComponent(MeshComponent);
    MeshComponent->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeAsset.Succeeded())
    {
        MeshComponent->SetStaticMesh(CubeAsset.Object);
        DebrisMesh = CubeAsset.Object;
    }
}

void AWPDestructibleCover::BeginPlay()
{
    Super::BeginPlay();

    Health = MaxHealth;
    OnTakeAnyDamage.AddDynamic(this, &AWPDestructibleCover::HandleTakeAnyDamage);
}

void AWPDestructibleCover::ConfigureCover(const FVector& InScale, float InMaxHealth)
{
    SetActorScale3D(InScale);
    MaxHealth = FMath::Max(1.0f, InMaxHealth);
    Health = MaxHealth;
}

void AWPDestructibleCover::HandleTakeAnyDamage(
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

    Health = FMath::Max(0.0f, Health - Damage);
    if (Health <= 0.0f)
    {
        BreakCover();
    }
}

void AWPDestructibleCover::BreakCover()
{
    if (bDestroyed || !GetWorld())
    {
        return;
    }

    bDestroyed = true;
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetVisibility(false, true);

    if (DebrisMesh)
    {
        const FVector OriginalScale = GetActorScale3D();

        for (int32 Index = 0; Index < 5; ++Index)
        {
            const FVector Jitter(
                FMath::FRandRange(-60.0f, 60.0f),
                FMath::FRandRange(-60.0f, 60.0f),
                FMath::FRandRange(25.0f, 90.0f));

            AStaticMeshActor* Debris = GetWorld()->SpawnActor<AStaticMeshActor>(
                GetActorLocation() + Jitter,
                FRotator(
                    FMath::FRandRange(-25.0f, 25.0f),
                    FMath::FRandRange(-180.0f, 180.0f),
                    FMath::FRandRange(-25.0f, 25.0f)));

            if (!Debris)
            {
                continue;
            }

            UStaticMeshComponent* DebrisComponent = Debris->GetStaticMeshComponent();
            DebrisComponent->SetMobility(EComponentMobility::Movable);
            DebrisComponent->SetStaticMesh(DebrisMesh);
            DebrisComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
            DebrisComponent->SetSimulatePhysics(true);

            const float PieceScale = FMath::FRandRange(0.12f, 0.23f);
            Debris->SetActorScale3D(OriginalScale * PieceScale);
            DebrisComponent->AddImpulse(
                FVector(
                    FMath::FRandRange(-420.0f, 420.0f),
                    FMath::FRandRange(-420.0f, 420.0f),
                    FMath::FRandRange(260.0f, 650.0f)),
                NAME_None,
                true);
            Debris->SetLifeSpan(8.0f);
        }
    }

    OnCoverDestroyed.Broadcast();
    SetLifeSpan(0.25f);
}
