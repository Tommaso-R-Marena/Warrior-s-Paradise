#pragma once

#include "CoreMinimal.h"
#include "Characters/WPSoldierCharacter.h"
#include "WPEnemySoldier.generated.h"

class UStaticMeshComponent;

UCLASS()
class WARRIORPARADISE_API AWPEnemySoldier : public AWPSoldierCharacter
{
    GENERATED_BODY()

public:
    AWPEnemySoldier();

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Visual")
    TObjectPtr<UStaticMeshComponent> TorsoVisual;

    UPROPERTY(VisibleAnywhere, Category="Visual")
    TObjectPtr<UStaticMeshComponent> HeadVisual;

    UPROPERTY(VisibleAnywhere, Category="Visual")
    TObjectPtr<UStaticMeshComponent> RifleVisual;

    UPROPERTY(EditDefaultsOnly, Category="AI")
    float DesiredRange = 850.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI")
    float MaximumEngagementRange = 2600.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI")
    float AimTurnSpeed = 4.5f;

    UPROPERTY(EditDefaultsOnly, Category="AI")
    float FirePulseSeconds = 0.22f;

    UPROPERTY(EditDefaultsOnly, Category="AI")
    float RetargetInterval = 0.45f;

    float NextDecisionTime = 0.0f;
    float NextTargetSearchTime = 0.0f;
    float StopFireTime = 0.0f;
    bool bPulseFiring = false;

    TWeakObjectPtr<AWPSoldierCharacter> CurrentTarget;

    AWPSoldierCharacter* FindBestTarget() const;
    bool HasLineOfSightTo(const AActor* Target) const;
};
