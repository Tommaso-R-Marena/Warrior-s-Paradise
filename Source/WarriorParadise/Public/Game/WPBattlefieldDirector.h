#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Actor.h"
#include "WPBattlefieldDirector.generated.h"

class APlayerController;
class AWPArtilleryStrike;
class AWPCaptureObjective;
class AWPCoverPoint;
class AWPEnemySoldier;
class AWPSoldierCharacter;
class UStaticMesh;

UCLASS()
class WARRIORPARADISE_API AWPBattlefieldDirector : public AActor
{
    GENERATED_BODY()

public:
    AWPBattlefieldDirector();

    UFUNCTION(BlueprintCallable, Category="Battle")
    void NotifySoldierKilled(AWPSoldierCharacter* Soldier);

    UFUNCTION(BlueprintCallable, Category="Battle|Squad")
    void IssueSquadOrder(EWPSquadOrder NewOrder, const FVector& OrderLocation);

    UFUNCTION(BlueprintCallable, Category="Battle|Support")
    bool RequestArtillery(AWPSoldierCharacter* Caller, const FVector& TargetLocation);

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetWave() const { return Wave; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetRemainingEnemies() const { return RemainingEnemies; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetFriendlySoldiersAlive() const { return FriendlySoldiersAlive; }

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

    UFUNCTION(BlueprintPure, Category="Battle|Squad")
    EWPSquadOrder GetSquadOrder() const { return CurrentSquadOrder; }

    UFUNCTION(BlueprintPure, Category="Battle|Squad")
    FVector GetSquadOrderLocation() const { return SquadOrderLocation; }

    UFUNCTION(BlueprintPure, Category="Battle|Support")
    int32 GetArtilleryCharges() const { return ArtilleryCharges; }

    UFUNCTION(BlueprintPure, Category="Battle|Support")
    float GetArtilleryCooldownRemaining() const;

    UFUNCTION(BlueprintPure, Category="Battle")
    bool IsFinalCounterattack() const { return bFinalCounterattack; }

    UFUNCTION(BlueprintPure, Category="Battle")
    bool IsBattleWon() const { return bBattleWon; }

    UFUNCTION(BlueprintPure, Category="Battle")
    bool IsBattleLost() const { return bBattleLost; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category="Battle")
    TSubclassOf<AWPEnemySoldier> AISoldierClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    TSubclassOf<AWPCaptureObjective> ObjectiveClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    TSubclassOf<AWPCoverPoint> CoverPointClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle|Support")
    TSubclassOf<AWPArtilleryStrike> ArtilleryStrikeClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    float InterWaveDelay = 4.5f;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    float FriendlyReplacementDelay = 6.5f;

    UPROPERTY(EditDefaultsOnly, Category="Battle|Support")
    float ArtilleryCooldownSeconds = 42.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 Wave = 1;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 RemainingEnemies = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 FriendlySoldiersAlive = 0;

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

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bFinalCounterattack = false;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bBattleWon = false;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bBattleLost = false;

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
    TWeakObjectPtr<APlayerController> PendingPlayerRespawnController;

    FTimerHandle NextWaveTimer;
    FTimerHandle FriendlyReplacementTimer;

    void BuildBattlefield();
    void BuildObjectives();
    void BuildCoverPoints();
    void SpawnFriendlySquad();
    AWPSoldierCharacter* SpawnFriendlySoldier(
        const FVector& Location,
        EWPSoldierRole Role,
        APlayerController* ControllerToPossess = nullptr);
    void SpawnQueuedFriendlyReplacement();
    FVector GetFriendlySpawnLocation() const;

    void SpawnWave();
    void SpawnEnemyGroup(int32 Count, bool bResetEnemyCounter);
    void SpawnNextWave();

    bool TransferPlayerControl(AWPSoldierCharacter* FallenSoldier);
    void QueueFriendlyReplacement(APlayerController* ControllerNeedingPawn);

    UFUNCTION()
    void HandleObjectiveCaptured(AWPCaptureObjective* Objective);

    void SpawnBox(
        const FVector& Location,
        const FVector& Scale,
        const FRotator& Rotation = FRotator::ZeroRotator);

    void SpawnCoverPoint(
        const FVector& Location,
        const FVector& CoverNormal,
        float Quality = 1.0f);
};
