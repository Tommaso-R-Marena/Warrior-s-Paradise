#include "Game/WPPlayerController.h"
#include "Characters/WPSoldierCharacter.h"
#include "Components/InputComponent.h"
#include "Game/WPGameUserSettings.h"
#include "GameFramework/Pawn.h"

AWPPlayerController::AWPPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    bEnableTouchEvents = true;
}

bool AWPPlayerController::IsMobileTouchEnabled() const
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    return IsLocalController();
#else
    return bPreviewMobileControlsInEditor && IsLocalController();
#endif
}

bool AWPPlayerController::IsMobileButtonHeld(EWPMobileButton Button) const
{
    for (const FWPMobileFingerState& State : Fingers)
    {
        if (State.bActive &&
            State.Gesture == EWPMobileGesture::Button &&
            State.Button == Button)
        {
            return true;
        }
    }
    return false;
}

void AWPPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (IsMobileTouchEnabled())
    {
        ProcessMobileTouch(DeltaSeconds);
    }
}

void AWPPlayerController::HandleTouchBegin(
    FWPMobileFingerState& State,
    const FVector2D& Position,
    const FVector2D& ViewSize,
    AWPSoldierCharacter* Soldier)
{
    State.bActive = true;
    State.LastPosition = Position;
    State.Anchor = Position;
    State.Button = WPMobileControls::HitTest(Position, ViewSize);
    if (State.Button != EWPMobileButton::None)
    {
        State.Gesture = EWPMobileGesture::Button;
        Soldier->ExecuteMobileAction(State.Button, true);
    }
    else if (Position.X < ViewSize.X * .41f &&
             Position.Y > ViewSize.Y * .40f)
    {
        State.Gesture = EWPMobileGesture::Move;
        MobileMoveAnchor = Position;
    }
    else
    {
        State.Gesture = EWPMobileGesture::Look;
    }
}

void AWPPlayerController::HandleTouchEnd(
    FWPMobileFingerState& State,
    AWPSoldierCharacter* Soldier)
{
    if (State.bActive &&
        State.Gesture == EWPMobileGesture::Button &&
        WPMobileControls::IsHeldAction(State.Button))
    {
        if (Soldier)
        {
            Soldier->ExecuteMobileAction(State.Button, false);
        }
    }
    State = FWPMobileFingerState{};
}

void AWPPlayerController::ReleaseAllMobileActions(
    AWPSoldierCharacter* PreviousSoldier)
{
    for (FWPMobileFingerState& State : Fingers)
    {
        HandleTouchEnd(State, PreviousSoldier);
    }
    MobileMoveStick = FVector2D::ZeroVector;
}

void AWPPlayerController::ProcessMobileTouch(float DeltaSeconds)
{
    if (!IsLocalController())
    {
        return;
    }

    AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(GetPawn());
    if (TrackedSoldier.Get() != Soldier)
    {
        ReleaseAllMobileActions(TrackedSoldier.Get());
        TrackedSoldier = Soldier;
    }

    if (!Soldier)
    {
        return;
    }

    int32 Width = 0;
    int32 Height = 0;
    GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0)
    {
        return;
    }

    const FVector2D ViewSize(static_cast<float>(Width), static_cast<float>(Height));
    MobileMoveAnchor = FVector2D(ViewSize.X * .17f, ViewSize.Y * .74f);
    MobileMoveStick = FVector2D::ZeroVector;

    // Sample all 10 simultaneous touch slots; any touch can become the
    // movement or look gesture, while the other fingers hold action buttons.
    for (int32 Index = 0; Index < MobileFingerCount; ++Index)
    {
        float TouchX = 0.0f;
        float TouchY = 0.0f;
        bool bPressed = false;
        GetInputTouchState(
            static_cast<ETouchIndex::Type>(Index),
            TouchX,
            TouchY,
            bPressed);

        FWPMobileFingerState& State = Fingers[Index];
        const FVector2D Position(TouchX, TouchY);

        if (!bPressed)
        {
            if (State.bActive)
            {
                HandleTouchEnd(State, Soldier);
            }
            continue;
        }

        if (!State.bActive)
        {
            HandleTouchBegin(State, Position, ViewSize, Soldier);
            continue;
        }

        if (State.Gesture == EWPMobileGesture::Look &&
            Soldier->IsCombatCapable())
        {
            const FVector2D Delta = Position - State.LastPosition;
            const float DegreesPerPixel =
                TouchLookSensitivity * 98.0f / FMath::Max(300.0f, ViewSize.Y);
            Soldier->AddControllerYawInput(
                FMath::Clamp(Delta.X, -85.0f, 85.0f) * DegreesPerPixel);
            Soldier->AddControllerPitchInput(
                -FMath::Clamp(Delta.Y, -85.0f, 85.0f) * DegreesPerPixel);
        }
        else if (State.Gesture == EWPMobileGesture::Move)
        {
            MobileMoveAnchor = State.Anchor;
            const float Radius = FMath::Max(50.0f,
                FMath::Min(ViewSize.X, ViewSize.Y) * .135f);
            const FVector2D Delta = Position - State.Anchor;
            const FVector2D RawStick = Delta.GetClampedToMaxSize(Radius) / Radius;
            const float Magnitude = RawStick.Size();
            MobileMoveStick = Magnitude > TouchDeadzone
                ? RawStick * ((Magnitude - TouchDeadzone) /
                              (Magnitude * (1.0f - TouchDeadzone)))
                : FVector2D::ZeroVector;
        }

        State.LastPosition = Position;
    }

    // The movement vector is applied per tick; combat actions remain
    // routed through AWPSoldierCharacter's actual stamina/ammo rules.
    if (Soldier->IsCombatCapable())
    {
        if (!FMath::IsNearlyZero(MobileMoveStick.Y))
        {
            Soldier->AddMovementInput(
                Soldier->GetActorForwardVector(), -MobileMoveStick.Y);
        }
        if (!FMath::IsNearlyZero(MobileMoveStick.X))
        {
            Soldier->AddMovementInput(
                Soldier->GetActorRightVector(), MobileMoveStick.X);
        }
    }
}

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
