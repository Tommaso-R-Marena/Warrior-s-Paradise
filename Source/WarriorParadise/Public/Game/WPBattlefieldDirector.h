#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WPBattlefieldDirector.generated.h"

class AWPEnemySoldier;
class UStaticMesh;

UCLASS()
class WARRIORPARADISE_API AWPBattlefieldDirector : public AActor
{
    GENERATED_BODY()

public:
    AWPBattlefieldDirector();

    UFUNCTION(BlueprintCallable, Category="Battle")
    void NotifyEnemyKilled(AActor* Enemy);

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetWave() const { return Wave; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetRemainingEnemies() const { return RemainingEnemies; }

    UFUNCTION(BlueprintPure, Category="Battle")
    int32 GetScore() const { return Score; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category="Battle")
    TSubclassOf<AWPEnemySoldier> EnemyClass;

    UPROPERTY(EditDefaultsOnly, Category="Battle")
    float InterWaveDelay = 4.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 Wave = 1;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 RemainingEnemies = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Battle")
    int32 Score = 0;

    TObjectPtr<UStaticMesh> CubeMesh;
    FTimerHandle NextWaveTimer;

    void BuildBattlefield();
    void SpawnWave();
    void SpawnNextWave();

    void SpawnBox(
        const FVector& Location,
        const FVector& Scale,
        const FRotator& Rotation = FRotator::ZeroRotator);
};
