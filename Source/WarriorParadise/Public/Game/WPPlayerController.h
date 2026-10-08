#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/WPMobileControls.h"
#include "WPPlayerController.generated.h"

class AWPSoldierCharacter;

enum class EWPMobileGesture : uint8
{
    None,
    Move,
    Look,
    Button
};

struct FWPMobileFingerState
{
    bool bActive = false;
    EWPMobileGesture Gesture = EWPMobileGesture::None;
    EWPMobileButton Button = EWPMobileButton::None;
    FVector2D Anchor = FVector2D::ZeroVector;
    FVector2D LastPosition = FVector2D::ZeroVector;
};

UCLASS()
class WARRIORPARADISE_API AWPPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AWPPlayerController();

    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="Mobile")
    bool IsMobileTouchEnabled() const;

    UFUNCTION(BlueprintPure, Category="Mobile")
    FVector2D GetMobileMoveStick() const { return MobileMoveStick; }

    UFUNCTION(BlueprintPure, Category="Mobile")
    FVector2D GetMobileMoveAnchor() const { return MobileMoveAnchor; }

    UFUNCTION(BlueprintPure, Category="Mobile")
    bool IsMobileButtonHeld(EWPMobileButton Button) const;

protected:
    virtual void SetupInputComponent() override;

    // Available for PIE testing on desktop. Enabled automatically when
    // compiled for Android or iOS, with no separate mobile Blueprint.
    UPROPERTY(EditDefaultsOnly, Category="Mobile")
    bool bPreviewMobileControlsInEditor = false;

    UPROPERTY(EditDefaultsOnly, Category="Mobile", meta=(ClampMin="0.1", ClampMax="4.0"))
    float TouchLookSensitivity = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category="Mobile", meta=(ClampMin="0.2", ClampMax="1.0"))
    float TouchDeadzone = 0.10f;

private:
    static constexpr int32 MobileFingerCount = 10;
    FWPMobileFingerState Fingers[MobileFingerCount];
    FVector2D MobileMoveStick = FVector2D::ZeroVector;
    FVector2D MobileMoveAnchor = FVector2D(.17f, .74f); // Pixels after Tick
    TWeakObjectPtr<AWPSoldierCharacter> TrackedSoldier;

    void CycleGraphicsPreset();
    void RunGraphicsAutoBenchmark();
    void ProcessMobileTouch(float DeltaSeconds);
    void ReleaseAllMobileActions(AWPSoldierCharacter* PreviousSoldier);
    void HandleTouchBegin(FWPMobileFingerState& State, const FVector2D& Position,
                          const FVector2D& ViewSize, AWPSoldierCharacter* Soldier);
    void HandleTouchEnd(FWPMobileFingerState& State, AWPSoldierCharacter* Soldier);
};
