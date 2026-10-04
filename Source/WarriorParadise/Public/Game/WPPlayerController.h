#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WPPlayerController.generated.h"

UCLASS()
class WARRIORPARADISE_API AWPPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void SetupInputComponent() override;

private:
    void CycleGraphicsPreset();
    void RunGraphicsAutoBenchmark();
};
