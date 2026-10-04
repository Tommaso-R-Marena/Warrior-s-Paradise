#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WPHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWPHealthChangedSignature, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FWPDamageReceivedSignature,
    float,
    Damage,
    AActor*,
    DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPDownedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPRevivedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPDeathSignature);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class WARRIORPARADISE_API UWPHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWPHealthComponent();

    UPROPERTY(BlueprintAssignable, Category="Health")
    FWPHealthChangedSignature OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category="Health|Presentation")
    FWPDamageReceivedSignature OnDamageReceived;

    UPROPERTY(BlueprintAssignable, Category="Health")
    FWPDownedSignature OnDowned;

    UPROPERTY(BlueprintAssignable, Category="Health")
    FWPRevivedSignature OnRevived;

    UPROPERTY(BlueprintAssignable, Category="Health")
    FWPDeathSignature OnDeath;

    UFUNCTION(BlueprintPure, Category="Health")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category="Health")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(BlueprintPure, Category="Health")
    float GetHealthNormalized() const { return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f; }

    UFUNCTION(BlueprintPure, Category="Health")
    bool IsDowned() const { return bDowned; }

    UFUNCTION(BlueprintPure, Category="Health")
    bool IsDead() const { return bDead; }

    UFUNCTION(BlueprintPure, Category="Health")
    float GetBleedoutRemaining() const;

    UFUNCTION(BlueprintPure, Category="Health")
    bool CanBeDowned() const { return bCanBeDowned; }

    UFUNCTION(BlueprintCallable, Category="Health")
    void SetCanBeDowned(bool bEnabled) { bCanBeDowned = bEnabled; }

    UFUNCTION(BlueprintCallable, Category="Health")
    void RestoreToFull();

    UFUNCTION(BlueprintCallable, Category="Health")
    bool Revive();

    UFUNCTION(BlueprintCallable, Category="Health")
    void ForceDeath();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category="Health", meta=(ClampMin="1.0"))
    float MaxHealth = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category="Health|Downed", meta=(ClampMin="1.0"))
    float ReviveHealth = 38.0f;

    UPROPERTY(EditDefaultsOnly, Category="Health|Downed", meta=(ClampMin="1.0"))
    float BleedoutSeconds = 19.0f;

    UPROPERTY(EditDefaultsOnly, Category="Health|Downed", meta=(ClampMin="0.0"))
    float DownedFinisherDamage = 14.0f;

    UPROPERTY(EditDefaultsOnly, Category="Health|Downed")
    bool bCanBeDowned = true;

    UPROPERTY(VisibleInstanceOnly, Category="Health")
    float Health = 100.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Health")
    bool bDowned = false;

    UPROPERTY(VisibleInstanceOnly, Category="Health")
    bool bDead = false;

    FTimerHandle BleedoutTimerHandle;

    UFUNCTION()
    void HandleTakeAnyDamage(
        AActor* DamagedActor,
        float Damage,
        const UDamageType* DamageType,
        AController* InstigatedBy,
        AActor* DamageCauser);

    void EnterDownedState();
    void FinalizeDeath();
};
