#include "Combat/WPHealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UWPHealthComponent::UWPHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWPHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    Health = MaxHealth;
    if (AActor* Owner = GetOwner())
    {
        Owner->OnTakeAnyDamage.AddDynamic(this, &UWPHealthComponent::HandleTakeAnyDamage);
    }
}

float UWPHealthComponent::GetBleedoutRemaining() const
{
    if (!bDowned || !GetWorld())
    {
        return 0.0f;
    }

    return FMath::Max(
        0.0f,
        GetWorld()->GetTimerManager().GetTimerRemaining(BleedoutTimerHandle));
}

void UWPHealthComponent::RestoreToFull()
{
    if (bDead || bDowned)
    {
        return;
    }

    const float Delta = MaxHealth - Health;
    Health = MaxHealth;
    OnHealthChanged.Broadcast(Health, Delta);
}

bool UWPHealthComponent::Revive()
{
    if (bDead || !bDowned)
    {
        return false;
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(BleedoutTimerHandle);
    }

    const float PreviousHealth = Health;
    bDowned = false;
    Health = FMath::Clamp(ReviveHealth, 1.0f, MaxHealth);
    OnHealthChanged.Broadcast(Health, Health - PreviousHealth);
    OnRevived.Broadcast();
    return true;
}

void UWPHealthComponent::ForceDeath()
{
    if (!bDead)
    {
        FinalizeDeath();
    }
}

void UWPHealthComponent::HandleTakeAnyDamage(
    AActor* DamagedActor,
    float Damage,
    const UDamageType* DamageType,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
    if (bDead || Damage <= 0.0f)
    {
        return;
    }

    if (bDowned)
    {
        if (Damage >= DownedFinisherDamage)
        {
            FinalizeDeath();
        }
        return;
    }

    const float PreviousHealth = Health;
    Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
    OnHealthChanged.Broadcast(Health, Health - PreviousHealth);

    if (Health <= 0.0f)
    {
        if (bCanBeDowned)
        {
            EnterDownedState();
        }
        else
        {
            FinalizeDeath();
        }
    }
}

void UWPHealthComponent::EnterDownedState()
{
    if (bDead || bDowned)
    {
        return;
    }

    bDowned = true;
    Health = 0.0f;
    OnDowned.Broadcast();

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            BleedoutTimerHandle,
            this,
            &UWPHealthComponent::FinalizeDeath,
            BleedoutSeconds,
            false);
    }
}

void UWPHealthComponent::FinalizeDeath()
{
    if (bDead)
    {
        return;
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(BleedoutTimerHandle);
    }

    bDead = true;
    bDowned = false;
    Health = 0.0f;
    OnDeath.Broadcast();
}
