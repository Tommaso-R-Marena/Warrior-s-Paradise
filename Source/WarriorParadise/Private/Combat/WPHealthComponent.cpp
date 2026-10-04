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

bool UWPHealthComponent::StopBleeding()
{
    if (!bBleeding)
    {
        return false;
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(
            BleedingTimerHandle);
    }

    bBleeding = false;
    BleedingSeverity = 0.0f;
    OnBleedingChanged.Broadcast(false, 0.0f);
    return true;
}

bool UWPHealthComponent::Heal(float Amount)
{
    if (bDead || bDowned || Amount <= 0.0f || Health >= MaxHealth)
    {
        return false;
    }

    const float PreviousHealth = Health;
    Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
    OnHealthChanged.Broadcast(Health, Health - PreviousHealth);
    return Health > PreviousHealth;
}

void UWPHealthComponent::RestoreToFull()
{
    if (bDead || bDowned)
    {
        return;
    }

    StopBleeding();

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
    StopBleeding();
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
    OnDamageReceived.Broadcast(Damage, DamageCauser);
    OnHealthChanged.Broadcast(Health, Health - PreviousHealth);

    if (Health > 0.0f)
    {
        TryStartBleedingFromDamage(Damage);
    }

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

void UWPHealthComponent::TryStartBleedingFromDamage(
    float Damage)
{
    if (bDead ||
        bDowned ||
        Damage <= 0.0f ||
        !GetWorld())
    {
        return;
    }

    const float HealthNormalized = GetHealthNormalized();
    const bool bTraumaticHit =
        Damage >= BleedingDamageThreshold;
    const bool bLowHealth =
        HealthNormalized <= LowHealthBleedingThreshold;

    if (!bTraumaticHit && !bLowHealth)
    {
        return;
    }

    const float DamageSeverity = FMath::GetMappedRangeValueClamped(
        FVector2D(
            BleedingDamageThreshold,
            FMath::Max(
                BleedingDamageThreshold + 1.0f,
                SevereBleedingDamageThreshold)),
        FVector2D(0.35f, 1.0f),
        Damage);

    const float HealthSeverity = bLowHealth
        ? FMath::GetMappedRangeValueClamped(
            FVector2D(
                0.0f,
                LowHealthBleedingThreshold),
            FVector2D(0.85f, 0.35f),
            HealthNormalized)
        : 0.0f;

    const float NewSeverity = FMath::Clamp(
        FMath::Max(
            BleedingSeverity,
            FMath::Max(
                bTraumaticHit ? DamageSeverity : 0.0f,
                HealthSeverity)),
        0.0f,
        1.0f);

    const bool bWasBleeding = bBleeding;
    const bool bSeverityChanged =
        !FMath::IsNearlyEqual(
            NewSeverity,
            BleedingSeverity);

    bBleeding = true;
    BleedingSeverity = NewSeverity;

    if (!GetWorld()->GetTimerManager().IsTimerActive(
            BleedingTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(
            BleedingTimerHandle,
            this,
            &UWPHealthComponent::TickBleeding,
            BleedingTickSeconds,
            true,
            BleedingTickSeconds);
    }

    if (!bWasBleeding || bSeverityChanged)
    {
        OnBleedingChanged.Broadcast(
            true,
            BleedingSeverity);
    }
}

void UWPHealthComponent::TickBleeding()
{
    if (!bBleeding ||
        bDead ||
        bDowned)
    {
        StopBleeding();
        return;
    }

    const float PreviousHealth = Health;
    const float BleedDamage =
        BleedingDamagePerSecond *
        BleedingSeverity *
        BleedingTickSeconds;

    Health = FMath::Clamp(
        Health - BleedDamage,
        0.0f,
        MaxHealth);

    if (!FMath::IsNearlyEqual(
            PreviousHealth,
            Health))
    {
        OnHealthChanged.Broadcast(
            Health,
            Health - PreviousHealth);
    }

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

    StopBleeding();

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
        GetWorld()->GetTimerManager().ClearTimer(BleedingTimerHandle);
    }

    if (bBleeding)
    {
        bBleeding = false;
        BleedingSeverity = 0.0f;
        OnBleedingChanged.Broadcast(false, 0.0f);
    }

    bDead = true;
    bDowned = false;
    Health = 0.0f;
    OnDeath.Broadcast();
}
