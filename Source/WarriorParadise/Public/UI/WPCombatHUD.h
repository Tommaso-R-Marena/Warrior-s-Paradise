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
    void DrawMobileTouchOverlay(float Width, float Height);
    void DrawGraphicsReadout(float Width);
    void DrawCombatReadout(float Width, float Height);
    void DrawTankReadout(float Width, float Height);
    void DrawVehicleReadout(float Width, float Height);
    void DrawEmplacementReadout(float Width, float Height);
};
