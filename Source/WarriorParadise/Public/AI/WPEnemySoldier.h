#pragma once

#include "CoreMinimal.h"
#include "Characters/WPSoldierCharacter.h"
#include "WPEnemySoldier.generated.h"

class AWPCoverPoint;
class AWPBattlefieldDirector;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EWPAITacticalPosture : uint8
{
    Independent UMETA(DisplayName="Independent"),
    BaseOfFire UMETA(DisplayName="Base Of Fire"),
    Maneuver UMETA(DisplayName="Maneuver"),
    Pinned UMETA(DisplayName="Pinned")
};

UCLASS()
class WARRIORPARADISE_API AWPEnemySoldier : public AWPSoldierCharacter
{
    GENERATED_BODY()

public:
    AWPEnemySoldier();

    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintPure, Category="AI|Tactics")
    EWPAITacticalPosture GetTacticalPosture() const
    {
        return TacticalPosture;
    }

    UFUNCTION(BlueprintPure, Category="AI|Tactics")
    bool HasUsedCoordinatedTactic() const
    {
        return bHasUsedCoordinatedTactic;
    }

    UFUNCTION(BlueprintPure, Category="AI|Tactics")
    bool HasHeldFireForFriendly() const
    {
        return bHasHeldFireForFriendly;
    }

    UFUNCTION(BlueprintPure, Category="AI|Tactics")
    bool HasHeldGrenadeForFriendly() const
    {
        return bHasHeldGrenadeForFriendly;
    }

    UFUNCTION(BlueprintPure, Category="AI|Tactics")
    bool HasUsedCoordinatedReload() const
    {
        return bHasUsedCoordinatedReload;
    }

    UFUNCTION(BlueprintPure, Category="AI|Tactics")
    bool HasWaitedForManeuverSupport() const
    {
        return bHasWaitedForManeuverSupport;
    }

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

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics", meta=(ClampMin="500.0"))
    float CoordinationRadius = 2400.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics", meta=(ClampMin="200.0"))
    float ManeuverFlankDistance = 850.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics", meta=(ClampMin="0.0"))
    float FireLaneSafetyRadius = 52.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics", meta=(ClampMin="0.0"))
    float FireLaneShiftStrength = 0.34f;

    UPROPERTY(
        EditDefaultsOnly,
        Category="AI|Tactics",
        meta=(ClampMin="0.0", ClampMax="1.0"))
    float TacticalReloadMagazineThreshold = 0.28f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics", meta=(ClampMin="100.0"))
    float ReloadCoveringTeammateRadius = 2200.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics", meta=(ClampMin="100.0"))
    float ReloadDangerInterruptRange = 520.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics", meta=(ClampMin="0.1"))
    float FireSuperiorityRecentFireWindow = 1.35f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics", meta=(ClampMin="1"))
    int32 MinimumManeuverCoveringShooters = 2;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics", meta=(ClampMin="1"))
    int32 MinimumSuppressedManeuverCoveringShooters = 1;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics", meta=(ClampMin="0.1"))
    float BaseOfFirePulseMultiplier = 1.55f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Grenade")
    float GrenadeMinimumRange = 650.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Grenade")
    float GrenadeMaximumRange = 1900.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Grenade")
    float GrenadeCooldownSeconds = 15.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Grenade", meta=(ClampMin="0.0"))
    float GrenadeFriendlySafetyRadius = 650.0f;

    float NextDecisionTime = 0.0f;
    float NextTargetSearchTime = 0.0f;
    float NextCoverSearchTime = 0.0f;
    float NextGrenadeTime = 0.0f;
    float NextSupportSearchTime = 0.0f;
    float NextEngineerSupplyTime = 0.0f;
    float StopFireTime = 0.0f;
    bool bPulseFiring = false;
    bool bHasUsedCoordinatedTactic = false;
    bool bHasHeldFireForFriendly = false;
    bool bHasHeldGrenadeForFriendly = false;
    bool bHasUsedCoordinatedReload = false;
    bool bHasWaitedForManeuverSupport = false;

    UPROPERTY(VisibleInstanceOnly, Category="AI|Tactics")
    EWPAITacticalPosture TacticalPosture =
        EWPAITacticalPosture::Independent;

    TWeakObjectPtr<AWPSoldierCharacter> CurrentTarget;
    TWeakObjectPtr<AWPCoverPoint> CurrentCover;
    TWeakObjectPtr<AWPBattlefieldDirector> BattlefieldDirector;

    AWPSoldierCharacter* FindBestTarget() const;
    AWPSoldierCharacter* FindDownedAlly(float MaximumRange) const;
    AWPSoldierCharacter* FindBleedingAlly(float MaximumRange) const;
    AWPSoldierCharacter* FindAmmoTarget(float MaximumRange) const;
    AWPCoverPoint* FindBestCover(const AWPSoldierCharacter* Threat) const;
    AWPEnemySoldier* SelectManeuverElement(
        const AWPSoldierCharacter* Target,
        int32& OutEligibleShooters) const;

    bool HasLineOfSightTo(const AActor* Target) const;
    bool HasClearFireLaneTo(
        const AWPSoldierCharacter* Target) const;
    bool IsGrenadeTargetSafe(
        const FVector& TargetLocation) const;
    bool HasCoveringTeammateForReload(
        const AWPSoldierCharacter* Target) const;
    bool ShouldTacticalReload(
        const AWPSoldierCharacter* Target,
        bool bAtCover,
        bool bTargetVisible) const;
    int32 CountManeuverCoveringShooters(
        const AWPSoldierCharacter* Target,
        const AWPEnemySoldier* ManeuverElement) const;
    bool HasFireSuperiorityForManeuver(
        const AWPSoldierCharacter* Target,
        const AWPEnemySoldier* ManeuverElement,
        int32& OutCoveringShooters) const;
    float GetManeuverSelectionScore() const;
    FVector GetStrategicDestination() const;
    FVector GetManeuverDestination(
        const AWPSoldierCharacter* Target) const;
    FVector GetFormationOffset() const;
    void MoveTacticallyToward(const FVector& Destination, float InputScale = 1.0f);
    void ReleaseCover();
    bool HandleMedicSupport();
    bool HandleEngineerSupport();
    void HandleGrenadeOpportunity(AWPSoldierCharacter* Target, float Distance);
};
