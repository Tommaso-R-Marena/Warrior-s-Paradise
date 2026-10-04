#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WPBattlefieldDirector.generated.h"

class AWPCaptureObjective;
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

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetWave() const { return Wave; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetRemainingEnemies() const { return RemainingEnemies; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetFriendlySoldiersAlive() const { return FriendlySoldiersAlive; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetScore() const { return Score; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetActiveObjectiveNumber() const;

    UFUNCTION(BlueprintPure, Category="Battle")
    float GetActiveObjectiveProgress() const;

    UFUNCTION(BlueprintPure, Category="Battle")
    bool IsFinalCounterattack() const { return bFinalCounterattack; }

    UFUNCTION(BlueprintPure, Category="Battle")
    bool IsBattleWon() const { return bBattleWon; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category="Battle")
    TSubclassOf<AWPEnemySoldier> AISoldierClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    TSubclassOf<AWPCaptureObjective> ObjectiveClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    float InterWaveDelay = 4.5f;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 Wave = 1;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 RemainingEnemies = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 FriendlySoldiersAlive = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 Score = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 ActiveObjectiveIndex = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bFinalCounterattack = false;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    bool bBattleWon = false;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AWPCaptureObjective>> Objectives;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AWPSoldierCharacter>> FriendlySquad;

    FTimerHandle NextWaveTimer;

    void BuildBattlefield();
    void BuildObjectives();
    void SpawnFriendlySquad();
    void SpawnWave();
    void SpawnEnemyGroup(int32 Count, bool bResetEnemyCounter);
    void SpawnNextWave();
    void TransferPlayerControl(AWPSoldierCharacter* FallenSoldier);

    UFUNCTION()
    void HandleObjectiveCaptured(AWPCaptureObjective* Objective);

    void SpawnBox(
        const FVector& Location,
        const FVector& Scale,
        const FRotator& Rotation = FRotator::ZeroRotator);
};
