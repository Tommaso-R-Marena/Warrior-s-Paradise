#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WPCombatHUD.generated.h"

UCLASS()
class WARRIORPARADISE_API AWPCombatHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    void DrawCrosshair(float CenterX, float CenterY);
    void DrawCombatReadout(float Width, float Height);
};
