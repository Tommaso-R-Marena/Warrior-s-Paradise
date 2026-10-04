#pragma once

#include "CoreMinimal.h"
#include "Characters/WPSoldierCharacter.h"
#include "WPEnemySoldier.generated.h"

class AWPCoverPoint;
class AWPBattlefieldDirector;
class UStaticMeshComponent;

UCLASS()
class WARRIORPARADISE_API AWPEnemySoldier : public AWPSoldierCharacter
{
    GENERATED_BODY()

public:
    AWPEnemySoldier();

    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
    float DesiredRange = 950.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI")
    float MaximumEngagementRange = 2800.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI")
    float AimTurnSpeed = 4.5f;

    UPROPERTY(EditDefaultsOnly, Category="AI")
    float FirePulseSeconds = 0.22f;

    UPROPERTY(EditDefaultsOnly, Category="AI")
    float RetargetInterval = 0.45f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover")
    float CoverSearchRadius = 1700.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover")
    float CoverTriggerSuppression = 0.38f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Grenade")
    float GrenadeMinimumRange = 650.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Grenade")
    float GrenadeMaximumRange = 1900.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Grenade")
    float GrenadeCooldownSeconds = 15.0f;

    float NextDecisionTime = 0.0f;
    float NextTargetSearchTime = 0.0f;
    float NextCoverSearchTime = 0.0f;
    float NextGrenadeTime = 0.0f;
    float NextSupportSearchTime = 0.0f;
    float NextEngineerSupplyTime = 0.0f;
    float StopFireTime = 0.0f;
    bool bPulseFiring = false;

    TWeakObjectPtr<AWPSoldierCharacter> CurrentTarget;
    TWeakObjectPtr<AWPCoverPoint> CurrentCover;
    TWeakObjectPtr<AWPBattlefieldDirector> BattlefieldDirector;

    AWPSoldierCharacter* FindBestTarget() const;
    AWPSoldierCharacter* FindDownedAlly(float MaximumRange) const;
    AWPSoldierCharacter* FindBleedingAlly(float MaximumRange) const;
    AWPSoldierCharacter* FindAmmoTarget(float MaximumRange) const;
    AWPCoverPoint* FindBestCover(const AWPSoldierCharacter* Threat) const;

    bool HasLineOfSightTo(const AActor* Target) const;
    FVector GetStrategicDestination() const;
    FVector GetFormationOffset() const;
    void MoveTacticallyToward(const FVector& Destination, float InputScale = 1.0f);
    void ReleaseCover();
    bool HandleMedicSupport();
    bool HandleEngineerSupport();
    void HandleGrenadeOpportunity(AWPSoldierCharacter* Target, float Distance);
};
