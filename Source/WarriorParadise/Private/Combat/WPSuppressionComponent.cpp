#include "Combat/WPSuppressionComponent.h"
#include "Engine/World.h"

UWPSuppressionComponent::UWPSuppressionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickInterval = 0.05f;
}

void UWPSuppressionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (Suppression <= 0.0f || !GetWorld())
    {
        return;
    }

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastSuppressionTime < DecayDelaySeconds)
    {
        return;
    }

    const float PreviousValue = Suppression;
    Suppression = FMath::Max(0.0f, Suppression - DecayPerSecond * DeltaTime);
    BroadcastIfChanged(PreviousValue);
}

void UWPSuppressionComponent::AddSuppression(float Amount)
{
    if (Amount <= 0.0f)
    {
        return;
    }

    const float PreviousValue = Suppression;
    Suppression = FMath::Clamp(Suppression + Amount, 0.0f, MaxSuppression);

    if (GetWorld())
    {
        LastSuppressionTime = GetWorld()->GetTimeSeconds();
    }

    BroadcastIfChanged(PreviousValue);
}

void UWPSuppressionComponent::ClearSuppression()
{
    const float PreviousValue = Suppression;
    Suppression = 0.0f;
    BroadcastIfChanged(PreviousValue);
}

void UWPSuppressionComponent::BroadcastIfChanged(float PreviousValue)
{
    UpdateSuppressionState();

    if (!FMath::IsNearlyEqual(PreviousValue, Suppression))
    {
        OnSuppressionChanged.Broadcast(GetNormalizedSuppression());
    }
}

void UWPSuppressionComponent::UpdateSuppressionState()
{
    const float Normalized = GetNormalizedSuppression();
    EWPSuppressionState NewState = SuppressionState;

    switch (SuppressionState)
    {
        case EWPSuppressionState::Steady:
            if (Normalized >= PinnedEnterThreshold)
            {
                NewState = EWPSuppressionState::Pinned;
            }
            else if (Normalized >= PressuredEnterThreshold)
            {
                NewState = EWPSuppressionState::Pressured;
            }
            break;

        case EWPSuppressionState::Pressured:
            if (Normalized >= PinnedEnterThreshold)
            {
                NewState = EWPSuppressionState::Pinned;
            }
            else if (Normalized <= PressuredExitThreshold)
            {
                NewState = EWPSuppressionState::Steady;
            }
            break;

        case EWPSuppressionState::Pinned:
            if (Normalized <= PressuredExitThreshold)
            {
                NewState = EWPSuppressionState::Steady;
            }
            else if (Normalized <= PinnedExitThreshold)
            {
                NewState = EWPSuppressionState::Pressured;
            }
            break;

        default:
            NewState = EWPSuppressionState::Steady;
            break;
    }

    if (NewState != SuppressionState)
    {
        SuppressionState = NewState;
        OnSuppressionStateChanged.Broadcast(SuppressionState);
    }
}
