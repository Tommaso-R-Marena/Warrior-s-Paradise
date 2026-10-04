#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Actor.h"
#include "WPBallisticProjectile.generated.h"

class AController;
class UPrimitiveComponent;
class USphereComponent;
class UProjectileMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FWPProjectileImpactSignature,
    FVector,
    ImpactLocation,
    FVector,
    ImpactNormal,
    AActor*,
    HitActor);

UCLASS()
class WARRIORPARADISE_API AWPBallisticProjectile : public AActor
{
    GENERATED_BODY()

public:
    AWPBallisticProjectile();

    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(BlueprintAssignable, Category="Projectile|Presentation")
    FWPProjectileImpactSignature OnImpact;

    void InitializeProjectile(
        const FVector& Direction,
        float MuzzleVelocityCmPerSecond,
        float InDamage,
        float InMaximumTravelCm,
        AController* InInstigatorController,
        EWPCombatTeam InTeam);

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

    UPROPERTY(EditDefaultsOnly, Category="Projectile|Penetration", meta=(ClampMin="0"))
    int32 MaximumPenetrations = 1;

    UPROPERTY(EditDefaultsOnly, Category="Projectile|Penetration", meta=(ClampMin="0.0", ClampMax="1.0"))
    float PenetrationDamageRetention = 0.62f;

    UPROPERTY(EditDefaultsOnly, Category="Projectile|Penetration", meta=(ClampMin="0.0", ClampMax="1.0"))
    float PenetrationVelocityRetention = 0.78f;

    float Damage = 34.0f;
    float MaximumTravelCm = 60000.0f;
    int32 PenetrationsRemaining = 1;
    FVector SpawnLocation = FVector::ZeroVector;
    EWPCombatTeam ProjectileTeam = EWPCombatTeam::Allies;
    TWeakObjectPtr<AController> InstigatorController;
    TSet<TWeakObjectPtr<AActor>> SuppressedActors;

    void ApplyNearMissSuppression();
    float GetHitLocationMultiplier(const FHitResult& Hit) const;
    bool TryPenetrate(AActor* OtherActor, const FHitResult& Hit);

    UFUNCTION()
    void HandleHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse,
        const FHitResult& Hit);
};
