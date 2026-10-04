#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "WPCombatGameMode.generated.h"

UCLASS()
class WARRIORPARADISE_API AWPCombatGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    AWPCombatGameMode();
    virtual void StartPlay() override;

private:
    void StartHeadlessSmokeTest();
    void FinishHeadlessSmokeTest();

    FTimerHandle HeadlessSmokeTimer;
    bool bHeadlessSmokeInitialStateValid = false;
};
