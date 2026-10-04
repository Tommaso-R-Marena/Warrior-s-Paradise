#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WPCoverPoint.generated.h"

class AWPSoldierCharacter;

UCLASS()
class WARRIORPARADISE_API AWPCoverPoint : public AActor
{
    GENERATED_BODY()

public:
    AWPCoverPoint();

    UFUNCTION(BlueprintCallable, Category="Cover")
    void InitializeCover(const FVector& InCoverNormal, float InQuality = 1.0f);

    UFUNCTION(BlueprintPure, Category="Cover")
    FVector GetCoverNormal() const { return CoverNormal; }

    UFUNCTION(BlueprintPure, Category="Cover")
    float GetCoverQuality() const { return CoverQuality; }

    bool IsAvailableFor(const AWPSoldierCharacter* Soldier) const;
    bool TryReserve(AWPSoldierCharacter* Soldier);
    void Release(AWPSoldierCharacter* Soldier);

private:
    UPROPERTY(VisibleInstanceOnly, Category="Cover")
    FVector CoverNormal = FVector::ForwardVector;

    UPROPERTY(VisibleInstanceOnly, Category="Cover")
    float CoverQuality = 1.0f;

    TWeakObjectPtr<AWPSoldierCharacter> Occupant;
};
