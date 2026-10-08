#include "Game/WPPlayerController.h"
#include "Characters/WPSoldierCharacter.h"
#include "Components/InputComponent.h"
#include "Game/WPGameUserSettings.h"
#include "GameFramework/Pawn.h"
#include "Vehicles/WPArmoredVehicle.h"
#include "Vehicles/WPLightVehicle.h"
#include "World/WPGunEmplacement.h"

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

EWPMobileMode AWPPlayerController::GetMobileMode(const APawn* Pawn) const
{
    if (Cast<AWPArmoredVehicle>(Pawn))
    {
        return EWPMobileMode::Tank;
    }
    if (Cast<AWPLightVehicle>(Pawn))
    {
        return EWPMobileMode::LightVehicle;
    }
    if (Cast<AWPGunEmplacement>(Pawn))
    {
        return EWPMobileMode::Emplacement;
    }
    return EWPMobileMode::Infantry;
}

void AWPPlayerController::DispatchMobileAction(
    APawn* Pawn,
    EWPMobileButton Button,
    bool bPressed)
{
    if (AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(Pawn))
    {
        Soldier->ExecuteMobileAction(Button, bPressed);
    }
    else if (AWPArmoredVehicle* Tank = Cast<AWPArmoredVehicle>(Pawn))
    {
        Tank->ExecuteMobileAction(Button, bPressed);
    }
    else if (AWPLightVehicle* LightVehicle = Cast<AWPLightVehicle>(Pawn))
    {
        LightVehicle->ExecuteMobileAction(Button, bPressed);
    }
    else if (AWPGunEmplacement* Gun = Cast<AWPGunEmplacement>(Pawn))
    {
        Gun->ExecuteMobileAction(Button, bPressed);
    }
}

void AWPPlayerController::HandleTouchBegin(
    FWPMobileFingerState& State,
    const FVector2D& Position,
    const FVector2D& ViewSize,
    APawn* ControlledPawn)
{
    State.bActive = true;
    State.LastPosition = Position;
    State.Anchor = Position;
    State.Button = WPMobileControls::HitTest(
        Position, ViewSize, GetMobileMode(ControlledPawn));
    if (State.Button != EWPMobileButton::None)
    {
        State.Gesture = EWPMobileGesture::Button;
        DispatchMobileAction(ControlledPawn, State.Button, true);
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
    APawn* ControlledPawn)
{
    if (State.bActive &&
        State.Gesture == EWPMobileGesture::Button &&
        WPMobileControls::IsHeldAction(State.Button))
    {
        DispatchMobileAction(ControlledPawn, State.Button, false);
    }
    State = FWPMobileFingerState{};
}

void AWPPlayerController::ReleaseAllMobileActions(APawn* PreviousPawn)
{
    for (FWPMobileFingerState& State : Fingers)
    {
        HandleTouchEnd(State, PreviousPawn);
    }
    MobileMoveStick = FVector2D::ZeroVector;
}

void AWPPlayerController::ProcessMobileTouch(float DeltaSeconds)
{
    if (!IsLocalController())
    {
        return;
    }

    APawn* ControlledPawn = GetPawn();
    if (TrackedMobilePawn.Get() != ControlledPawn)
    {
        // Possession can change mid-tap (entering a tank or swapping a
        // squadmate). Release the previous pawn's held fire/ADS first.
        ReleaseAllMobileActions(TrackedMobilePawn.Get());
        TrackedMobilePawn = ControlledPawn;
    }

    if (!Cast<AWPSoldierCharacter>(ControlledPawn) &&
        !Cast<AWPArmoredVehicle>(ControlledPawn) &&
        !Cast<AWPLightVehicle>(ControlledPawn) &&
        !Cast<AWPGunEmplacement>(ControlledPawn))
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

    for (int32 Index = 0; Index < MobileFingerCount; ++Index)
    {
        float TouchX = 0.0f;
        float TouchY = 0.0f;
        bool bPressed = false;
        GetInputTouchState(static_cast<ETouchIndex::Type>(Index),
                           TouchX, TouchY, bPressed);

        FWPMobileFingerState& State = Fingers[Index];
        const FVector2D Position(TouchX, TouchY);
        if (!bPressed)
        {
            if (State.bActive)
            {
                HandleTouchEnd(State, ControlledPawn);
            }
            continue;
        }

        if (!State.bActive)
        {
            HandleTouchBegin(State, Position, ViewSize, ControlledPawn);
            continue;
        }

        if (State.Gesture == EWPMobileGesture::Look)
        {
            const FVector2D Delta = Position - State.LastPosition;
            const float DegreesPerPixel =
                TouchLookSensitivity * 98.0f / FMath::Max(300.0f, ViewSize.Y);
            const float Yaw = FMath::Clamp(Delta.X, -85.0f, 85.0f) * DegreesPerPixel;
            const float Pitch = -FMath::Clamp(Delta.Y, -85.0f, 85.0f) * DegreesPerPixel;
            if (AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(ControlledPawn))
            {
                if (Soldier->IsCombatCapable())
                {
                    Soldier->AddControllerYawInput(Yaw);
                    Soldier->AddControllerPitchInput(Pitch);
                }
            }
            else if (AWPArmoredVehicle* Tank = Cast<AWPArmoredVehicle>(ControlledPawn))
            {
                Tank->ApplyMobileLook(Yaw, Pitch);
            }
            else if (AWPLightVehicle* Vehicle = Cast<AWPLightVehicle>(ControlledPawn))
            {
                Vehicle->ApplyMobileLook(Yaw, Pitch);
            }
            else if (AWPGunEmplacement* Gun = Cast<AWPGunEmplacement>(ControlledPawn))
            {
                Gun->ApplyMobileLook(Yaw, Pitch);
            }
        }
        else if (State.Gesture == EWPMobileGesture::Move)
        {
            MobileMoveAnchor = State.Anchor;
            const float Radius = FMath::Max(50.0f,
                FMath::Min(ViewSize.X, ViewSize.Y) * .135f);
            const FVector2D Delta = Position - State.Anchor;
            const FVector2D RawStick = Delta.GetClampedToMaxSize(Radius) / Radius;
            const float Magnitude = RawStick.Size();
            const float Deadzone = FMath::Clamp(TouchDeadzone, 0.0f, .95f);
            MobileMoveStick = Magnitude > Deadzone
                ? RawStick * ((Magnitude - Deadzone) /
                              (Magnitude * (1.0f - Deadzone)))
                : FVector2D::ZeroVector;
        }

        State.LastPosition = Position;
    }

    const float Forward = -MobileMoveStick.Y;
    const float Steering = MobileMoveStick.X;

    if (AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(ControlledPawn))
    {
        if (Soldier->IsCombatCapable())
        {
            if (!FMath::IsNearlyZero(Forward))
            {
                Soldier->AddMovementInput(Soldier->GetActorForwardVector(), Forward);
            }
            if (!FMath::IsNearlyZero(Steering))
            {
                Soldier->AddMovementInput(Soldier->GetActorRightVector(), Steering);
            }
        }
    }
    else if (AWPArmoredVehicle* Tank = Cast<AWPArmoredVehicle>(ControlledPawn))
    {
        Tank->ApplyMobileMovement(Forward, Steering);
    }
    else if (AWPLightVehicle* Vehicle = Cast<AWPLightVehicle>(ControlledPawn))
    {
        Vehicle->ApplyMobileMovement(Forward, Steering);
    }
    else if (AWPGunEmplacement* Gun = Cast<AWPGunEmplacement>(ControlledPawn))
    {
        Gun->ApplyMobileMovement(Forward, Steering);
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
