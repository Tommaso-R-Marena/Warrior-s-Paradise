#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "Presentation/WPEnvironmentVisualProfile.h"
#include "GameFramework/Actor.h"
#include "WPBattlefieldDirector.generated.h"

class APlayerController;
class AWPArtilleryStrike;
class AWPAirSupportAircraft;
class AWPArmoredVehicle;
class AWPCaptureObjective;
class AWPCoverPoint;
class AWPEnemySoldier;
class AWPGunEmplacement;
class AWPLightVehicle;
class AWPSoldierCharacter;
class UAudioComponent;
class UMaterialInterface;
class UStaticMesh;
class UWPBattlePresentationSet;
class UWPExplosionPresentationProfile;
class UWPSmokePresentationProfile;

UCLASS()
class WARRIORPARADISE_API AWPBattlefieldDirector : public AActor
{
    GENERATED_BODY()

public:
    AWPBattlefieldDirector();

    UFUNCTION(BlueprintCallable, Category="Battle")
    void NotifySoldierKilled(AWPSoldierCharacter* Soldier);

    UFUNCTION(BlueprintCallable, Category="Battle")
    void NotifyVehicleDestroyed(EWPCombatTeam VehicleTeam);
    void NotifyLightVehicleDestroyed(EWPCombatTeam VehicleTeam);

    UFUNCTION(BlueprintCallable, Category="Battle|Squad")
    void IssueSquadOrder(EWPSquadOrder NewOrder, const FVector& OrderLocation);

    UFUNCTION(BlueprintCallable, Category="Battle|Squad")
    bool SwitchPlayerToSquadmate(AWPSoldierCharacter* CurrentSoldier);

    UFUNCTION(BlueprintCallable, Category="Battle|Support")
    bool RequestArtillery(AWPSoldierCharacter* Caller, const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category="Battle|Support")
    bool RequestAirSupport(
        AWPSoldierCharacter* Caller,
        const FVector& TargetLocation);

    void ReportEnemyContact(EWPCombatTeam ReportingTeam, AWPSoldierCharacter* Contact);
    bool GetRecentEnemyContactLocation(
        EWPCombatTeam ReportingTeam,
        FVector& OutLocation,
        float MaximumAgeSeconds = 4.0f) const;

    void ReportGunshot(
        EWPCombatTeam ShooterTeam,
        const FVector& Location,
        float AudibleRadiusCm = 9000.0f);

    bool GetRecentEnemyGunshot(
        EWPCombatTeam ListeningTeam,
        const FVector& ListenerLocation,
        FVector& OutLocation,
        float MaximumAgeSeconds = 4.5f) const;

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetWave() const { return Wave; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetRemainingEnemies() const { return RemainingEnemies; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetFriendlySoldiersAlive() const { return FriendlySoldiersAlive; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetActiveEnemyVehicles() const { return ActiveEnemyVehicles; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetActiveEnemyLightVehicles() const { return ActiveEnemyLightVehicles; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetAlliedTickets() const { return AlliedTickets; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetAxisTickets() const { return AxisTickets; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetScore() const { return Score; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetActiveObjectiveNumber() const;

    UFUNCTION(BlueprintPure, Category="Battle")
    float GetActiveObjectiveProgress() const;

    UFUNCTION(BlueprintPure, Category="Battle")
    FVector GetActiveObjectiveLocation() const;

    UFUNCTION(BlueprintPure, Category="Battle")
    bool IsActiveObjectiveContested() const;

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetActiveObjectiveAlliedPresence() const;

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetActiveObjectiveAxisPresence() const;

    UFUNCTION(BlueprintPure, Category="Battle|Squad")
    EWPSquadOrder GetSquadOrder() const { return CurrentSquadOrder; }

    UFUNCTION(BlueprintPure, Category="Battle|Squad")
    FVector GetSquadOrderLocation() const { return SquadOrderLocation; }

    UFUNCTION(BlueprintPure, Category="Battle|Support")
    int32 GetArtilleryCharges() const { return ArtilleryCharges; }

    UFUNCTION(BlueprintPure, Category="Battle|Support")
    float GetArtilleryCooldownRemaining() const;

    UFUNCTION(BlueprintPure, Category="Battle|Support")
    int32 GetAirSupportCharges() const { return AirSupportCharges; }

    UFUNCTION(BlueprintPure, Category="Battle|Support")
    float GetAirSupportCooldownRemaining() const;

    UFUNCTION(BlueprintPure, Category="Battle")
    bool IsFinalCounterattack() const { return bFinalCounterattack; }

    UFUNCTION(BlueprintPure, Category="Battle")
    bool IsBattleWon() const { return bBattleWon; }

    UFUNCTION(BlueprintPure, Category="Battle")
    bool IsBattleLost() const { return bBattleLost; }

    UFUNCTION(BlueprintPure, Category="Battle|Presentation")
    UWPExplosionPresentationProfile* GetFragExplosionPresentation() const;

    UFUNCTION(BlueprintPure, Category="Battle|Presentation")
    UWPExplosionPresentationProfile* GetArtilleryExplosionPresentation() const;

    UFUNCTION(BlueprintPure, Category="Battle|Presentation")
    UWPSmokePresentationProfile* GetSmokePresentation() const;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category="Presentation")
    TSoftObjectPtr<UWPBattlePresentationSet> BattlePresentationSetAsset;

    UPROPERTY(Transient)
    TObjectPtr<UWPBattlePresentationSet> BattlePresentationSet;

    UPROPERTY(Transient)
    TObjectPtr<UWPEnvironmentVisualProfile> EnvironmentVisualProfile;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    TSubclassOf<AWPEnemySoldier> AISoldierClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    TSubclassOf<AWPCaptureObjective> ObjectiveClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    TSubclassOf<AWPCoverPoint> CoverPointClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle|Support")
    TSubclassOf<AWPArtilleryStrike> ArtilleryStrikeClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle|Vehicles")
    TSubclassOf<AWPArmoredVehicle> ArmoredVehicleClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle|Vehicles")
    TSubclassOf<AWPLightVehicle> LightVehicleClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle|Emplacements")
    TSubclassOf<AWPGunEmplacement> GunEmplacementClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle|Air Support")
    TSubclassOf<AWPAirSupportAircraft> AirSupportClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    float InterWaveDelay = 4.5f;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    float FriendlyReplacementDelay = 6.5f;

    UPROPERTY(EditDefaultsOnly, Category="Battle|Support")
    float ArtilleryCooldownSeconds = 42.0f;

    UPROPERTY(EditDefaultsOnly, Category="Battle|Air Support")
    float AirSupportCooldownSeconds = 70.0f;

    UPROPERTY(EditDefaultsOnly, Category="Battle|Air Support")
    float AmbientFlybyMinIntervalSeconds = 42.0f;

    UPROPERTY(EditDefaultsOnly, Category="Battle|Air Support")
    float AmbientFlybyMaxIntervalSeconds = 76.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 Wave = 1;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 RemainingEnemies = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 FriendlySoldiersAlive = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 ActiveEnemyVehicles = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 ActiveEnemyLightVehicles = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 AlliedTickets = 14;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 AxisTickets = 64;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 Score = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 ActiveObjectiveIndex = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle|Squad")
    EWPSquadOrder CurrentSquadOrder = EWPSquadOrder::AttackObjective;

    UPROPERTY(VisibleInstanceOnly, Category="Battle|Squad")
    FVector SquadOrderLocation = FVector::ZeroVector;

    UPROPERTY(VisibleInstanceOnly, Category="Battle|Support")
    int32 ArtilleryCharges = 2;

    UPROPERTY(VisibleInstanceOnly, Category="Battle|Air Support")
    int32 AirSupportCharges = 2;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bFinalCounterattack = false;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bAlliedArmorSpawned = false;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bAxisArmorSpawned = false;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bAlliedLightVehiclesSpawned = false;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bAxisLightVehiclesSpawned = false;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bBattleWon = false;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bBattleLost = false;

    bool bHeadlessSimulation = false;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AWPCaptureObjective>> Objectives;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AWPCoverPoint>> CoverPoints;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AWPSoldierCharacter>> FriendlySquad;

    int32 PendingFriendlyReplacements = 0;
    int32 ReplacementRoleCursor = 0;
    float NextArtilleryAvailableTime = 0.0f;
    float NextAirSupportAvailableTime = 0.0f;
    TWeakObjectPtr<APlayerController> PendingPlayerRespawnController;
    FVector AlliedSharedContactLocation = FVector::ZeroVector;
    FVector AxisSharedContactLocation = FVector::ZeroVector;
    float AlliedSharedContactTime = -1000.0f;
    float AxisSharedContactTime = -1000.0f;
    FVector AlliedGunshotLocation = FVector::ZeroVector;
    FVector AxisGunshotLocation = FVector::ZeroVector;
    float AlliedGunshotTime = -1000.0f;
    float AxisGunshotTime = -1000.0f;
    float AlliedGunshotRadiusCm = 0.0f;
    float AxisGunshotRadiusCm = 0.0f;

    FTimerHandle NextWaveTimer;
    FTimerHandle FriendlyReplacementTimer;
    FTimerHandle DistantBattleTimer;
    FTimerHandle AmbientFlybyTimer;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> AmbientAudioComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> WindAudioComponent;

    void BuildBattlefield();
    void BuildAtmosphere();
    void StartEnvironmentAudio();
    void PlayDistantBattleOneShot();
    void ScatterEnvironmentProps();
    void BuildObjectives();
    void BuildCoverPoints();
    void SpawnInitialCombinedArms();
    void SpawnDefensiveEmplacements();
    void SpawnFriendlySquad();
    AWPSoldierCharacter* SpawnFriendlySoldier(
        const FVector& Location,
        EWPSoldierRole Role,
        APlayerController* ControllerToPossess = nullptr);
    void SpawnQueuedFriendlyReplacement();
    FVector GetFriendlySpawnLocation() const;
    bool IsFriendlySpawnLocationSafe(const FVector& Location) const;

    void SpawnWave();
    void SpawnEnemyGroup(int32 Count, bool bResetEnemyCounter);
    void SpawnNextWave();
    AWPArmoredVehicle* SpawnArmoredVehicle(
        EWPCombatTeam Team,
        const FVector& Location,
        const FRotator& Rotation);

    AWPLightVehicle* SpawnLightVehicle(
        EWPCombatTeam Team,
        EWPLightVehicleType Type,
        const FVector& Location,
        const FRotator& Rotation);

    AWPGunEmplacement* SpawnGunEmplacement(
        EWPCombatTeam Team,
        EWPEmplacementType Type,
        const FVector& Location,
        const FRotator& Rotation);

    AWPAirSupportAircraft* SpawnAirSupport(
        EWPCombatTeam Team,
        const FVector& TargetLocation,
        EWPAirSupportMission Mission);

    void ScheduleAmbientFlyby();

    UFUNCTION()
    void SpawnAmbientFlyby();

    bool TransferPlayerControl(AWPSoldierCharacter* FallenSoldier);
    void QueueFriendlyReplacement(APlayerController* ControllerNeedingPawn);

    UFUNCTION()
    void HandleObjectiveCaptured(AWPCaptureObjective* Objective);

    UMaterialInterface* ResolveSurfaceMaterial(EWPEnvironmentSurface Surface) const;
    bool SpawnBuildingVisual(
        const FVector& Location,
        const FVector& BoxScale,
        const FRotator& Rotation);

    void SpawnEnterableBuilding(
        const FVector& Location,
        const FVector& Scale,
        const FRotator& Rotation,
        EWPEnvironmentSurface WallSurface);

    void SpawnBox(
        const FVector& Location,
        const FVector& Scale,
        const FRotator& Rotation = FRotator::ZeroRotator,
        EWPEnvironmentSurface Surface = EWPEnvironmentSurface::Concrete,
        bool bCollisionOnly = false);

    void SpawnCoverPoint(
        const FVector& Location,
        const FVector& CoverNormal,
        float Quality = 1.0f);

    void SpawnDestructibleCover(
        const FVector& Location,
        const FVector& Scale,
        float Health = 180.0f,
        EWPEnvironmentSurface Surface = EWPEnvironmentSurface::Wood);
};
