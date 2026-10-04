#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Actor.h"
#include "WPTankShell.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class WARRIORPARADISE_API AWPTankShell : public AActor
{
    GENERATED_BODY()

public:
    AWPTankShell();

    void InitializeShell(EWPCombatTeam InTeam, const FVector& Velocity);

private:
    UPROPERTY(VisibleAnywhere, Category="Shell")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category="Shell")
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY(EditDefaultsOnly, Category="Shell")
    float MaximumDamage = 320.0f;

    UPROPERTY(EditDefaultsOnly, Category="Shell")
    float BlastRadiusCm = 600.0f;

    UPROPERTY(EditDefaultsOnly, Category="Shell")
    float SuppressionRadiusCm = 1100.0f;

    EWPCombatTeam ShellTeam = EWPCombatTeam::Allies;

    UFUNCTION()
    void HandleHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse,
        const FHitResult& Hit);

    void Explode(const FVector& ImpactLocation);
};
