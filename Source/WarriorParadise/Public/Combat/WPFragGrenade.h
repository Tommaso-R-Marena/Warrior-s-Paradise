#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Actor.h"
#include "WPFragGrenade.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWPExplosionSignature, FVector, Location);

UCLASS()
class WARRIORPARADISE_API AWPFragGrenade : public AActor
{
    GENERATED_BODY()

public:
    AWPFragGrenade();

    UPROPERTY(BlueprintAssignable, Category="Grenade")
    FWPExplosionSignature OnExploded;

    void InitializeGrenade(EWPCombatTeam InTeam, const FVector& LaunchVelocity);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Grenade")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category="Grenade")
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY(EditDefaultsOnly, Category="Grenade", meta=(ClampMin="0.1"))
    float FuseSeconds = 3.1f;

    UPROPERTY(EditDefaultsOnly, Category="Grenade", meta=(ClampMin="1.0"))
    float MaximumDamage = 115.0f;

    UPROPERTY(EditDefaultsOnly, Category="Grenade", meta=(ClampMin="1.0"))
    float BlastRadiusCm = 550.0f;

    UPROPERTY(EditDefaultsOnly, Category="Grenade", meta=(ClampMin="1.0"))
    float SuppressionRadiusCm = 1000.0f;

    EWPCombatTeam GrenadeTeam = EWPCombatTeam::Allies;
    FTimerHandle FuseTimerHandle;

    void Explode();
};
