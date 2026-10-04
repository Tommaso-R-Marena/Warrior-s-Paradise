#include "Combat/WPHealthComponent.h"
#include "GameFramework/Actor.h"

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

void UWPHealthComponent::RestoreToFull()
{
    if (bDead)
    {
        return;
    }

    const float Delta = MaxHealth - Health;
    Health = MaxHealth;
    OnHealthChanged.Broadcast(Health, Delta);
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

    const float PreviousHealth = Health;
    Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
    OnHealthChanged.Broadcast(Health, Health - PreviousHealth);

    if (Health <= 0.0f)
    {
        bDead = true;
        OnDeath.Broadcast();
    }
}
