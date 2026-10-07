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

    UFUNCTION(BlueprintPure, Category="AI|Tactics")
    bool HasUsedCoverPosture() const
    {
        return bHasUsedCoverPosture;
    }

    UFUNCTION(BlueprintPure, Category="AI|Tactics")
    bool HasUsedManeuverSmoke() const
    {
        return bHasUsedManeuverSmoke;
    }

    UFUNCTION(BlueprintPure, Category="AI|Tactics")
    bool HasAdvancedUnderManeuverSmoke() const
    {
        return bHasAdvancedUnderManeuverSmoke;
    }

    UFUNCTION(BlueprintPure, Category="AI|Cover")
    bool HasSelectedPeekCapableCover() const
    {
        return bHasSelectedPeekCapableCover;
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

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover", meta=(ClampMin="0.1"))
    float CoverPeekDuration = 0.72f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover", meta=(ClampMin="0.1"))
    float CoverHideDuration = 0.58f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover", meta=(ClampMin="0.0"))
    float CoverPostureJitter = 0.18f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover", meta=(ClampMin="0.1"))
    float CoverContactMemorySeconds = 1.8f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover", meta=(ClampMin="0.0"))
    float CoverCrouchedEyeHeight = 38.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover", meta=(ClampMin="0.0"))
    float CoverStandingEyeHeight = 68.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover", meta=(ClampMin="0.0"))
    float CoverPeekCapabilityBonus = 700.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover", meta=(ClampMin="0.0"))
    float CoverDeepConcealmentPenalty = 180.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover", meta=(ClampMin="0.0"))
    float CoverExposedPenalty = 1250.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Cover", meta=(ClampMin="0.0"))
    float CoverApproachExposurePenalty = 420.0f;

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

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics|Smoke", meta=(ClampMin="0.0"))
    float ManeuverSmokeWaitSeconds = 1.15f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics|Smoke", meta=(ClampMin="0.1"))
    float ManeuverSmokeDeployDelay = 3.1f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics|Smoke", meta=(ClampMin="1.0"))
    float ManeuverSmokeWindowSeconds = 12.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics|Smoke", meta=(ClampMin="100.0"))
    float ManeuverSmokeLateralOffset = 520.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI|Tactics|Smoke", meta=(ClampMin="0.0"))
    float ManeuverSmokeVerificationTolerance = 250.0f;

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
    float NextCoverPostureChangeTime = 0.0f;
    float LastVisibleTargetTime = -1000.0f;
    float ManeuverSupportWaitStartTime = -1.0f;
    float ManeuverSmokeReadyTime = -1.0f;
    float ManeuverSmokeExpireTime = -1.0f;
    FVector ManeuverSmokeTargetLocation = FVector::ZeroVector;
    FVector ManeuverSmokeFlankDestination = FVector::ZeroVector;
    bool bPulseFiring = false;
    bool bPeekingFromCover = false;
    bool bHasUsedCoordinatedTactic = false;
    bool bHasHeldFireForFriendly = false;
    bool bHasHeldGrenadeForFriendly = false;
    bool bHasUsedCoordinatedReload = false;
    bool bHasWaitedForManeuverSupport = false;
    bool bHasUsedCoverPosture = false;
    bool bHasUsedManeuverSmoke = false;
    bool bHasAdvancedUnderManeuverSmoke = false;
    bool bHasSelectedPeekCapableCover = false;

    UPROPERTY(VisibleInstanceOnly, Category="AI|Tactics")
    EWPAITacticalPosture TacticalPosture =
        EWPAITacticalPosture::Independent;

    TWeakObjectPtr<AWPSoldierCharacter> CurrentTarget;
    TWeakObjectPtr<AWPSoldierCharacter> ManeuverSupportWaitTarget;
    TWeakObjectPtr<AWPSoldierCharacter> SmokeManeuverTarget;
    TWeakObjectPtr<AWPCoverPoint> CurrentCover;
    TWeakObjectPtr<AWPBattlefieldDirector> BattlefieldDirector;

    AWPSoldierCharacter* FindBestTarget() const;
    AWPSoldierCharacter* FindDownedAlly(float MaximumRange) const;
    AWPSoldierCharacter* FindBleedingAlly(float MaximumRange) const;
    AWPSoldierCharacter* FindAmmoTarget(float MaximumRange) const;
    AWPCoverPoint* FindBestCover(const AWPSoldierCharacter* Threat);
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
    bool TryPrepareSmokeAssistedManeuver(
        AWPSoldierCharacter* Target,
        float Time);
    bool IsSmokeAssistedManeuverReady(float Time) const;
    float GetManeuverSelectionScore() const;
    FVector GetStrategicDestination() const;
    FVector GetManeuverDestination(
        const AWPSoldierCharacter* Target) const;
    FVector GetManeuverSmokeTarget(
        const AWPSoldierCharacter* Target) const;
    FVector GetFormationOffset() const;
    void MoveTacticallyToward(const FVector& Destination, float InputScale = 1.0f);
    void ApplyAIStance(bool bCrouch, bool bGoProne);
    void UpdateCoverPosture(
        bool bAtCover,
        bool bPinned,
        float Time);
    void ReleaseCover();
    bool HandleMedicSupport();
    bool HandleEngineerSupport();
    void HandleGrenadeOpportunity(AWPSoldierCharacter* Target, float Distance);
};
