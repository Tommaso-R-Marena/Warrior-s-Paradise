#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Actor.h"
#include "WPArtilleryStrike.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWPArtilleryImpactSignature, FVector, ImpactLocation);

UCLASS()
class WARRIORPARADISE_API AWPArtilleryStrike : public AActor
{
    GENERATED_BODY()

public:
    AWPArtilleryStrike();

    UPROPERTY(BlueprintAssignable, Category="Artillery")
    FWPArtilleryImpactSignature OnImpact;

    void InitializeStrike(const FVector& InTargetLocation, EWPCombatTeam InTeam);

private:
    UPROPERTY(EditDefaultsOnly, Category="Artillery", meta=(ClampMin="1"))
    int32 ShellCount = 8;

    UPROPERTY(EditDefaultsOnly, Category="Artillery", meta=(ClampMin="0.05"))
    float ShellIntervalSeconds = 0.55f;

    UPROPERTY(EditDefaultsOnly, Category="Artillery", meta=(ClampMin="0.0"))
    float InitialDelaySeconds = 1.65f;

    UPROPERTY(EditDefaultsOnly, Category="Artillery", meta=(ClampMin="10.0"))
    float ScatterRadiusCm = 720.0f;

    UPROPERTY(EditDefaultsOnly, Category="Artillery", meta=(ClampMin="1.0"))
    float BlastRadiusCm = 620.0f;

    UPROPERTY(EditDefaultsOnly, Category="Artillery", meta=(ClampMin="1.0"))
    float MaximumDamage = 145.0f;

    UPROPERTY(EditDefaultsOnly, Category="Artillery", meta=(ClampMin="1.0"))
    float SuppressionRadiusCm = 1250.0f;

    FVector TargetLocation = FVector::ZeroVector;
    EWPCombatTeam StrikeTeam = EWPCombatTeam::Allies;
    int32 ShellsFired = 0;
    FTimerHandle ShellTimerHandle;

    void FireImpact();
    FVector ChooseImpactLocation() const;
};
