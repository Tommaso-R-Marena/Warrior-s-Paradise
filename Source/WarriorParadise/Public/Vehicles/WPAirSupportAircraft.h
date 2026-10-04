#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Actor.h"
#include "WPAirSupportAircraft.generated.h"

class AWPAirBomb;
class UAudioComponent;
class UStaticMeshComponent;
class USoundBase;
class UWPAirSupportVisualProfile;
class UWPExplosionPresentationProfile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPAirSupportSimpleEventSignature);

UCLASS()
class WARRIORPARADISE_API AWPAirSupportAircraft : public AActor
{
    GENERATED_BODY()

public:
    AWPAirSupportAircraft();

    virtual void Tick(float DeltaSeconds) override;

    void InitializeMission(
        EWPCombatTeam InTeam,
        EWPAirSupportMission InMission,
        const FVector& InTargetLocation,
        UWPAirSupportVisualProfile* InVisualProfile,
        UWPExplosionPresentationProfile* InBombExplosionProfile);

    UFUNCTION(BlueprintPure, Category="Air Support")
    EWPCombatTeam GetCombatTeam() const { return CombatTeam; }

    UFUNCTION(BlueprintPure, Category="Air Support")
    EWPAirSupportMission GetMission() const { return Mission; }

    UPROPERTY(BlueprintAssignable, Category="Air Support|Presentation")
    FWPAirSupportSimpleEventSignature OnAttackStarted;

    UPROPERTY(BlueprintAssignable, Category="Air Support|Presentation")
    FWPAirSupportSimpleEventSignature OnMissionCompleted;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Aircraft")
    TObjectPtr<UStaticMeshComponent> AircraftVisual;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> EngineAudioComponent;

    UPROPERTY(Transient)
    TObjectPtr<UWPAirSupportVisualProfile> VisualProfile;

    UPROPERTY(Transient)
    TObjectPtr<UWPExplosionPresentationProfile> BombExplosionProfile;

    UPROPERTY(EditDefaultsOnly, Category="Aircraft")
    float FlightSpeedCmPerSecond = 2500.0f;

    UPROPERTY(EditDefaultsOnly, Category="Aircraft")
    float AttackAltitudeCm = 2250.0f;

    UPROPERTY(EditDefaultsOnly, Category="Aircraft|Strafe")
    int32 StrafeRounds = 20;

    UPROPERTY(EditDefaultsOnly, Category="Aircraft|Strafe")
    float StrafeIntervalSeconds = 0.085f;

    UPROPERTY(EditDefaultsOnly, Category="Aircraft|Bombing")
    int32 BombCount = 3;

    UPROPERTY(EditDefaultsOnly, Category="Aircraft|Bombing")
    float BombIntervalSeconds = 0.38f;

    EWPCombatTeam CombatTeam = EWPCombatTeam::Allies;
    EWPAirSupportMission Mission = EWPAirSupportMission::Flyby;
    FVector TargetLocation = FVector::ZeroVector;
    FVector FlightDirection = FVector::ForwardVector;
    FVector EndLocation = FVector::ZeroVector;
    bool bAttackStarted = false;
    int32 RemainingAttackUnits = 0;

    FTimerHandle AttackTimer;

    void BeginAttack();
    void FireStrafeRound();
    void DropBomb();
    void FinishAttack();
    void CompleteMission();
    void PlayOneShot(USoundBase* Sound, float Volume) const;
};
