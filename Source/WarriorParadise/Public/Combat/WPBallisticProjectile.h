#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WPBallisticProjectile.generated.h"

class AController;
class UPrimitiveComponent;
class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class WARRIORPARADISE_API AWPBallisticProjectile : public AActor
{
    GENERATED_BODY()

public:
    AWPBallisticProjectile();

    virtual void Tick(float DeltaSeconds) override;

    void InitializeProjectile(
        const FVector& Direction,
        float MuzzleVelocityCmPerSecond,
        float InDamage,
        float InMaximumTravelCm,
        AController* InInstigatorController);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Projectile")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category="Projectile")
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY(EditDefaultsOnly, Category="Projectile|Suppression", meta=(ClampMin="0.0"))
    float NearMissRadiusCm = 175.0f;

    UPROPERTY(EditDefaultsOnly, Category="Projectile|Suppression", meta=(ClampMin="0.0"))
    float NearMissSuppression = 13.0f;

    float Damage = 34.0f;
    float MaximumTravelCm = 60000.0f;
    FVector SpawnLocation = FVector::ZeroVector;
    TWeakObjectPtr<AController> InstigatorController;
    TSet<TWeakObjectPtr<AActor>> SuppressedActors;

    void ApplyNearMissSuppression();

    UFUNCTION()
    void HandleHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse,
        const FHitResult& Hit);
};
