#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WPDestructibleCover.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWPCoverDestroyedSignature);

UCLASS()
class WARRIORPARADISE_API AWPDestructibleCover : public AActor
{
    GENERATED_BODY()

public:
    AWPDestructibleCover();

    UPROPERTY(BlueprintAssignable, Category="Destruction")
    FWPCoverDestroyedSignature OnCoverDestroyed;

    UFUNCTION(BlueprintCallable, Category="Destruction")
    void ConfigureCover(const FVector& InScale, float InMaxHealth = 180.0f);

    UFUNCTION(BlueprintCallable, Category="Destruction|Presentation")
    void ApplyMaterial(UMaterialInterface* Material);

    UFUNCTION(BlueprintPure, Category="Destruction")
    float GetHealthNormalized() const
    {
        return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
    }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Destruction")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> DebrisMesh;

    UPROPERTY(EditDefaultsOnly, Category="Destruction", meta=(ClampMin="1.0"))
    float MaxHealth = 180.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Destruction")
    float Health = 180.0f;

    bool bDestroyed = false;

    UFUNCTION()
    void HandleTakeAnyDamage(
        AActor* DamagedActor,
        float Damage,
        const UDamageType* DamageType,
        AController* InstigatedBy,
        AActor* DamageCauser);

    void BreakCover();
};
