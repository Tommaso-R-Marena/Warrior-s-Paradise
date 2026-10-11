#pragma once

#include "CoreMinimal.h"

// A single normalized layout drives BOTH input hit testing and HUD drawing.
// This avoids the common mobile failure where visible buttons cannot be tapped.
enum class EWPMobileButton : uint8
{
    None,
    Fire,
    Aim,
    Reload,
    Frag,
    Smoke,
    Bandage,
    Switch,
    Interact,
    Crouch,
    Prone,
    Sprint,
    Squad,
    Jump
};

enum class EWPMobileMode : uint8
{
    Infantry,
    Tank,
    LightVehicle,
    Emplacement
};

struct FWPMobileButtonSpec
{
    EWPMobileButton Action;
    const TCHAR* Label;
    FVector2D Center;   // Normalized landscape viewport
    FVector2D HalfSize; // Normalized viewport
};

namespace WPMobileControls
{
    inline const FWPMobileButtonSpec* GetButtons(int32& OutCount)
    {
        static const FWPMobileButtonSpec Buttons[] =
        {
            {EWPMobileButton::Fire, TEXT("FIRE"), FVector2D(.89f, .67f), FVector2D(.080f, .115f)},
            {EWPMobileButton::Aim, TEXT("ADS"), FVector2D(.76f, .72f), FVector2D(.047f, .077f)},
            {EWPMobileButton::Reload, TEXT("RLD"), FVector2D(.92f, .46f), FVector2D(.038f, .067f)},
            {EWPMobileButton::Frag, TEXT("FRAG"), FVector2D(.82f, .46f), FVector2D(.038f, .067f)},
            {EWPMobileButton::Smoke, TEXT("SMK"), FVector2D(.72f, .46f), FVector2D(.038f, .067f)},
            {EWPMobileButton::Bandage, TEXT("MED"), FVector2D(.67f, .86f), FVector2D(.038f, .065f)},
            {EWPMobileButton::Switch, TEXT("SWAP"), FVector2D(.78f, .89f), FVector2D(.039f, .064f)},
            {EWPMobileButton::Interact, TEXT("USE"), FVector2D(.57f, .75f), FVector2D(.041f, .067f)},
            {EWPMobileButton::Crouch, TEXT("CRO"), FVector2D(.58f, .58f), FVector2D(.038f, .062f)},
            {EWPMobileButton::Prone, TEXT("PRO"), FVector2D(.58f, .91f), FVector2D(.038f, .063f)},
            {EWPMobileButton::Sprint, TEXT("RUN"), FVector2D(.34f, .66f), FVector2D(.040f, .074f)},
            {EWPMobileButton::Squad, TEXT("TEAM"), FVector2D(.46f, .91f), FVector2D(.042f, .063f)},
            {EWPMobileButton::Jump, TEXT("JUMP"), FVector2D(.92f, .89f), FVector2D(.048f, .066f)}
        };
        OutCount = UE_ARRAY_COUNT(Buttons);
        return Buttons;
    }

    inline bool IsInside(const FVector2D& NormalizedPos, const FWPMobileButtonSpec& Button)
    {
        return FMath::Abs(NormalizedPos.X - Button.Center.X) <= Button.HalfSize.X &&
               FMath::Abs(NormalizedPos.Y - Button.Center.Y) <= Button.HalfSize.Y;
    }

    inline bool IsAvailable(EWPMobileButton Action, EWPMobileMode Mode)
    {
        if (Mode == EWPMobileMode::Infantry)
        {
            return Action != EWPMobileButton::None;
        }
        if (Mode == EWPMobileMode::Tank)
        {
            return Action == EWPMobileButton::Fire ||
                   Action == EWPMobileButton::Aim ||
                   Action == EWPMobileButton::Reload ||
                   Action == EWPMobileButton::Interact;
        }
        if (Mode == EWPMobileMode::LightVehicle)
        {
            // Vehicle gunfire and disembark are distinct from infantry actions.
            return Action == EWPMobileButton::Fire ||
                   Action == EWPMobileButton::Interact;
        }
        if (Mode == EWPMobileMode::Emplacement)
        {
            // Mounted guns expose fire and leave controls.
            return Action == EWPMobileButton::Fire ||
                   Action == EWPMobileButton::Interact;
        }
        return false;
    }

    inline const TCHAR* GetLabel(EWPMobileButton Action, EWPMobileMode Mode,
                                 const TCHAR* InfantryLabel)
    {
        if (Mode == EWPMobileMode::Tank)
        {
            switch (Action)
            {
                case EWPMobileButton::Fire: return TEXT("CANNON");
                case EWPMobileButton::Aim: return TEXT("MG");
                case EWPMobileButton::Reload: return TEXT("SHELL");
                case EWPMobileButton::Interact: return TEXT("EXIT");
                default: break;
            }
        }
        if (Mode != EWPMobileMode::Infantry &&
            Action == EWPMobileButton::Interact)
        {
            return TEXT("EXIT");
        }
        return InfantryLabel;
    }

    inline EWPMobileButton HitTest(const FVector2D& PixelPosition,
                                  const FVector2D& ViewSize,
                                  EWPMobileMode Mode = EWPMobileMode::Infantry)
    {
        if (ViewSize.X <= 0.0f || ViewSize.Y <= 0.0f)
        {
            return EWPMobileButton::None;
        }
        const FVector2D Normalized(PixelPosition.X / ViewSize.X, PixelPosition.Y / ViewSize.Y);
        int32 Count = 0;
        const FWPMobileButtonSpec* Buttons = GetButtons(Count);
        for (int32 Index = 0; Index < Count; ++Index)
        {
            if (IsAvailable(Buttons[Index].Action, Mode) &&
                IsInside(Normalized, Buttons[Index]))
            {
                return Buttons[Index].Action;
            }
        }
        return EWPMobileButton::None;
    }

    inline bool IsHeldAction(EWPMobileButton Action)
    {
        return Action == EWPMobileButton::Fire ||
               Action == EWPMobileButton::Aim ||
               Action == EWPMobileButton::Sprint;
    }
}
