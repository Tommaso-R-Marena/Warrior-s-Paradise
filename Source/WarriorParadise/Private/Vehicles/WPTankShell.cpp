#include "Vehicles/WPTankShell.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSuppressionComponent.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/WPArmoredVehicle.h"
#include "Vehicles/WPLightVehicle.h"
#include "World/WPDestructibleCover.h"
#include "World/WPGunEmplacement.h"

AWPTankShell::AWPTankShell()
{
    PrimaryActorTick.bCanEverTick = false;
    InitialLifeSpan = 8.0f;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(5.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Block);
    Collision->SetNotifyRigidBodyCollision(true);
    Collision->OnComponentHit.AddDynamic(this, &AWPTankShell::HandleHit);

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->bRotationFollowsVelocity = true;
    Movement->bShouldBounce = false;
    Movement->ProjectileGravityScale = 0.72f;
    Movement->InitialSpeed = 22000.0f;
    Movement->MaxSpeed = 26000.0f;
}

void AWPTankShell::InitializeShell(
    EWPCombatTeam InTeam,
    const FVector& Velocity,
    EWPArmorShellType InShellType)
{
    ShellTeam = InTeam;
    ShellType = InShellType;

    if (ShellType == EWPArmorShellType::HighExplosive)
    {
        MaximumDamage = HEMaximumDamage;
        DirectArmorDamage = HEDirectArmorDamage;
        BlastRadiusCm = HEBlastRadiusCm;
    }
    else
    {
        MaximumDamage = APMaximumDamage;
        DirectArmorDamage = APDirectArmorDamage;
        BlastRadiusCm = APBlastRadiusCm;
    }

    Movement->Velocity = Velocity;
    Movement->InitialSpeed = Velocity.Size();
    Movement->MaxSpeed = FMath::Max(Movement->InitialSpeed, 26000.0f);
}

void AWPTankShell::HandleHit(
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

    if (const AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(OtherActor))
    {
        if (Soldier->GetCombatTeam() == ShellTeam)
        {
            Destroy();
            return;
        }
    }

    if (AWPArmoredVehicle* Vehicle = Cast<AWPArmoredVehicle>(OtherActor))
    {
        if (Vehicle->GetCombatTeam() == ShellTeam)
        {
            Destroy();
            return;
        }

        Vehicle->ApplyShellImpact(
            DirectArmorDamage,
            Hit.ImpactPoint,
            Movement->Velocity.GetSafeNormal(),
            ShellType);

        Explode(Hit.ImpactPoint, Vehicle);
        return;
    }

    if (AWPLightVehicle* Vehicle = Cast<AWPLightVehicle>(OtherActor))
    {
        if (Vehicle->GetCombatTeam() == ShellTeam)
        {
            Destroy();
            return;
        }

        const float DirectDamage =
            ShellType == EWPArmorShellType::ArmorPiercing
                ? DirectArmorDamage * 0.92f
                : MaximumDamage * 0.82f;

        UGameplayStatics::ApplyDamage(
            Vehicle,
            DirectDamage,
            nullptr,
            this,
            UDamageType::StaticClass());

        Explode(Hit.ImpactPoint, Vehicle);
        return;
    }

    if (AWPGunEmplacement* Gun = Cast<AWPGunEmplacement>(OtherActor))
    {
        if (Gun->GetCombatTeam() == ShellTeam)
        {
            Destroy();
            return;
        }

        UGameplayStatics::ApplyDamage(
            Gun,
            ShellType == EWPArmorShellType::HighExplosive
                ? MaximumDamage
                : DirectArmorDamage * 0.55f,
            nullptr,
            this,
            UDamageType::StaticClass());

        Explode(Hit.ImpactPoint, Gun);
        return;
    }

    Explode(Hit.ImpactPoint, OtherActor);
}

void AWPTankShell::Explode(const FVector& ImpactLocation, AActor* DirectHitActor)
{
    if (!GetWorld())
    {
        Destroy();
        return;
    }

    for (TActorIterator<AWPSoldierCharacter> It(GetWorld()); It; ++It)
    {
        AWPSoldierCharacter* Soldier = *It;
        if (!Soldier ||
            Soldier->GetCombatTeam() == ShellTeam ||
            !Soldier->GetHealthComponent() ||
            Soldier->GetHealthComponent()->IsDead())
        {
            continue;
        }

        const float Distance = FVector::Dist(ImpactLocation, Soldier->GetActorLocation());

        if (Distance <= SuppressionRadiusCm && Soldier->GetSuppressionComponent())
        {
            const float Alpha = 1.0f - FMath::Clamp(Distance / SuppressionRadiusCm, 0.0f, 1.0f);
            Soldier->GetSuppressionComponent()->AddSuppression(100.0f * Alpha);
        }

        if (Distance <= BlastRadiusCm)
        {
            const float Alpha = 1.0f - FMath::Clamp(Distance / BlastRadiusCm, 0.0f, 1.0f);
            UGameplayStatics::ApplyDamage(
                Soldier,
                MaximumDamage * FMath::Square(Alpha),
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    for (TActorIterator<AWPArmoredVehicle> It(GetWorld()); It; ++It)
    {
        AWPArmoredVehicle* Vehicle = *It;
        if (!Vehicle ||
            Vehicle->GetCombatTeam() == ShellTeam ||
            Vehicle == GetOwner() ||
            Vehicle == DirectHitActor)
        {
            continue;
        }

        const float Distance = FVector::Dist(ImpactLocation, Vehicle->GetActorLocation());
        if (Distance <= BlastRadiusCm)
        {
            const float Alpha = 1.0f - FMath::Clamp(Distance / BlastRadiusCm, 0.0f, 1.0f);
            UGameplayStatics::ApplyDamage(
                Vehicle,
                MaximumDamage * FMath::Lerp(0.45f, 1.0f, Alpha),
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    for (TActorIterator<AWPLightVehicle> It(GetWorld()); It; ++It)
    {
        AWPLightVehicle* Vehicle = *It;
        if (!Vehicle ||
            Vehicle->GetCombatTeam() == ShellTeam ||
            Vehicle == GetOwner() ||
            Vehicle == DirectHitActor ||
            Vehicle->IsDestroyed())
        {
            continue;
        }

        const float Distance = FVector::Dist(
            ImpactLocation,
            Vehicle->GetActorLocation());

        if (Distance <= BlastRadiusCm)
        {
            const float Alpha =
                1.0f -
                FMath::Clamp(
                    Distance / BlastRadiusCm,
                    0.0f,
                    1.0f);

            UGameplayStatics::ApplyDamage(
                Vehicle,
                MaximumDamage *
                    FMath::Lerp(0.28f, 0.72f, Alpha),
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    for (TActorIterator<AWPGunEmplacement> It(GetWorld()); It; ++It)
    {
        AWPGunEmplacement* Gun = *It;
        if (!Gun ||
            Gun->GetCombatTeam() == ShellTeam ||
            Gun == DirectHitActor ||
            Gun->IsDestroyed())
        {
            continue;
        }

        const float Distance = FVector::Dist(
            ImpactLocation,
            Gun->GetActorLocation());

        if (Distance <= BlastRadiusCm)
        {
            const float Alpha =
                1.0f -
                FMath::Clamp(
                    Distance / BlastRadiusCm,
                    0.0f,
                    1.0f);

            UGameplayStatics::ApplyDamage(
                Gun,
                MaximumDamage * FMath::Lerp(0.35f, 0.9f, Alpha),
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

        const float Distance = FVector::Dist(ImpactLocation, Cover->GetActorLocation());
        if (Distance <= BlastRadiusCm)
        {
            const float Alpha = 1.0f - FMath::Clamp(Distance / BlastRadiusCm, 0.0f, 1.0f);
            UGameplayStatics::ApplyDamage(
                Cover,
                MaximumDamage * 0.9f * Alpha,
                nullptr,
                this,
                UDamageType::StaticClass());
        }
    }

    OnImpact.Broadcast(ImpactLocation);
    Destroy();
}
