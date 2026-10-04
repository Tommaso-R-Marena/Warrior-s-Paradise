#include "Game/WPPlayerController.h"
#include "Components/InputComponent.h"
#include "Game/WPGameUserSettings.h"

void AWPPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!InputComponent)
    {
        return;
    }

    InputComponent->BindAction(
        TEXT("CycleGraphicsPreset"),
        IE_Pressed,
        this,
        &AWPPlayerController::CycleGraphicsPreset);

    InputComponent->BindAction(
        TEXT("AutoGraphicsBenchmark"),
        IE_Pressed,
        this,
        &AWPPlayerController::RunGraphicsAutoBenchmark);
}

void AWPPlayerController::CycleGraphicsPreset()
{
    if (UWPGameUserSettings* Settings =
        UWPGameUserSettings::Get())
    {
        Settings->CycleGraphicsPreset();
    }
}

void AWPPlayerController::RunGraphicsAutoBenchmark()
{
    if (UWPGameUserSettings* Settings =
        UWPGameUserSettings::Get())
    {
        Settings->RunAutoBenchmark();
    }
}
