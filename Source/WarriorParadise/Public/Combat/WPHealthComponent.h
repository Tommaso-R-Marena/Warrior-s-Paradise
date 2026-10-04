#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WPHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWPHealthChangedSignature, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPDeathSignature);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class WARRIORPARADISE_API UWPHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWPHealthComponent();

    UPROPERTY(BlueprintAssignable, Category="Health")
    FWPHealthChangedSignature OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category="Health")
    FWPDeathSignature OnDeath;

    UFUNCTION(BlueprintPure, Category="Health")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category="Health")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(BlueprintPure, Category="Health")
    float GetHealthNormalized() const { return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f; }

    UFUNCTION(BlueprintPure, Category="Health")
    bool IsDead() const { return bDead; }

    UFUNCTION(BlueprintCallable, Category="Health")
    void RestoreToFull();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category="Health", meta=(ClampMin="1.0"))
    float MaxHealth = 100.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Health")
    float Health = 100.0f;

    bool bDead = false;

    UFUNCTION()
    void HandleTakeAnyDamage(
        AActor* DamagedActor,
        float Damage,
        const UDamageType* DamageType,
        AController* InstigatedBy,
        AActor* DamageCauser);
};
