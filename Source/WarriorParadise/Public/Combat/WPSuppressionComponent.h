#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WPSuppressionComponent.generated.h"

UENUM(BlueprintType)
enum class EWPSuppressionState : uint8
{
    Steady UMETA(DisplayName="Steady"),
    Pressured UMETA(DisplayName="Pressured"),
    Pinned UMETA(DisplayName="Pinned")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FWPSuppressionChangedSignature,
    float,
    NormalizedSuppression);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FWPSuppressionStateChangedSignature,
    EWPSuppressionState,
    NewState);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class WARRIORPARADISE_API UWPSuppressionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWPSuppressionComponent();

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(BlueprintAssignable, Category="Suppression")
    FWPSuppressionChangedSignature OnSuppressionChanged;

    UPROPERTY(BlueprintAssignable, Category="Suppression")
    FWPSuppressionStateChangedSignature OnSuppressionStateChanged;

    UFUNCTION(BlueprintCallable, Category="Suppression")
    void AddSuppression(float Amount);

    UFUNCTION(BlueprintCallable, Category="Suppression")
    void ClearSuppression();

    UFUNCTION(BlueprintPure, Category="Suppression")
    float GetSuppression() const { return Suppression; }

    UFUNCTION(BlueprintPure, Category="Suppression")
    float GetNormalizedSuppression() const
    {
        return MaxSuppression > 0.0f ? Suppression / MaxSuppression : 0.0f;
    }

    UFUNCTION(BlueprintPure, Category="Suppression")
    EWPSuppressionState GetSuppressionState() const
    {
        return SuppressionState;
    }

    UFUNCTION(BlueprintPure, Category="Suppression")
    bool IsPinned() const
    {
        return SuppressionState == EWPSuppressionState::Pinned;
    }

private:
    UPROPERTY(EditDefaultsOnly, Category="Suppression", meta=(ClampMin="1.0"))
    float MaxSuppression = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category="Suppression", meta=(ClampMin="0.0"))
    float DecayDelaySeconds = 1.15f;

    UPROPERTY(EditDefaultsOnly, Category="Suppression", meta=(ClampMin="0.0"))
    float DecayPerSecond = 21.0f;

    UPROPERTY(EditDefaultsOnly, Category="Suppression|State", meta=(ClampMin="0.0", ClampMax="1.0"))
    float PressuredEnterThreshold = 0.35f;

    UPROPERTY(EditDefaultsOnly, Category="Suppression|State", meta=(ClampMin="0.0", ClampMax="1.0"))
    float PressuredExitThreshold = 0.22f;

    UPROPERTY(EditDefaultsOnly, Category="Suppression|State", meta=(ClampMin="0.0", ClampMax="1.0"))
    float PinnedEnterThreshold = 0.72f;

    UPROPERTY(EditDefaultsOnly, Category="Suppression|State", meta=(ClampMin="0.0", ClampMax="1.0"))
    float PinnedExitThreshold = 0.55f;

    UPROPERTY(VisibleInstanceOnly, Category="Suppression")
    float Suppression = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Suppression")
    EWPSuppressionState SuppressionState = EWPSuppressionState::Steady;

    float LastSuppressionTime = -1000.0f;

    void BroadcastIfChanged(float PreviousValue);
    void UpdateSuppressionState();
};
