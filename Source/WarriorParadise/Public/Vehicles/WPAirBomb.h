#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Actor.h"
#include "WPAirBomb.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UWPExplosionPresentationProfile;

UCLASS()
class WARRIORPARADISE_API AWPAirBomb : public AActor
{
    GENERATED_BODY()

public:
    AWPAirBomb();

    void InitializeBomb(
        EWPCombatTeam InTeam,
        const FVector& InitialVelocity,
        UWPExplosionPresentationProfile* InPresentationProfile);

private:
    UPROPERTY(VisibleAnywhere, Category="Bomb")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category="Bomb")
    TObjectPtr<UStaticMeshComponent> Visual;

    UPROPERTY(VisibleAnywhere, Category="Bomb")
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY(Transient)
    TObjectPtr<UWPExplosionPresentationProfile> PresentationProfile;

    UPROPERTY(EditDefaultsOnly, Category="Bomb")
    float MaximumInfantryDamage = 520.0f;

    UPROPERTY(EditDefaultsOnly, Category="Bomb")
    float VehicleDamage = 260.0f;

    UPROPERTY(EditDefaultsOnly, Category="Bomb")
    float BlastRadiusCm = 850.0f;

    UPROPERTY(EditDefaultsOnly, Category="Bomb")
    float SuppressionRadiusCm = 1500.0f;

    EWPCombatTeam BombTeam = EWPCombatTeam::Allies;
    bool bExploded = false;

    UFUNCTION()
    void HandleHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse,
        const FHitResult& Hit);

    void Explode(const FVector& ImpactLocation);
};
