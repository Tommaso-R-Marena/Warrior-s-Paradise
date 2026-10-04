#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Actor.h"
#include "WPCaptureObjective.generated.h"

class AWPCaptureObjective;
class UPointLightComponent;
class USphereComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FWPObjectiveCapturedSignature,
    AWPCaptureObjective*,
    Objective);

UCLASS()
class WARRIORPARADISE_API AWPCaptureObjective : public AActor
{
    GENERATED_BODY()

public:
    AWPCaptureObjective();

    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(BlueprintAssignable, Category="Objective")
    FWPObjectiveCapturedSignature OnCaptured;

    void InitializeObjective(int32 InObjectiveIndex);

    UFUNCTION(BlueprintCallable, Category="Objective")
    void SetObjectiveActive(bool bNewActive);

    UFUNCTION(BlueprintPure, Category="Objective")
    int32 GetObjectiveIndex() const { return ObjectiveIndex; }

    UFUNCTION(BlueprintPure, Category="Objective")
    float GetCaptureProgress() const { return CaptureProgress; }

    UFUNCTION(BlueprintPure, Category="Objective")
    bool IsObjectiveActive() const { return bActive; }

    UFUNCTION(BlueprintPure, Category="Objective")
    bool IsCaptured() const { return bCaptured; }

private:
    UPROPERTY(VisibleAnywhere, Category="Objective")
    TObjectPtr<USphereComponent> Zone;

    UPROPERTY(VisibleAnywhere, Category="Objective")
    TObjectPtr<UPointLightComponent> BeaconLight;

    UPROPERTY(VisibleAnywhere, Category="Objective")
    TObjectPtr<UTextRenderComponent> ObjectiveLabel;

    UPROPERTY(EditDefaultsOnly, Category="Objective", meta=(ClampMin="100.0"))
    float CaptureRadius = 650.0f;

    UPROPERTY(EditDefaultsOnly, Category="Objective", meta=(ClampMin="1.0"))
    float SoloCaptureSeconds = 11.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Objective")
    int32 ObjectiveIndex = 0;

    UPROPERTY(VisibleInstanceOnly, Category="Objective")
    float CaptureProgress = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Objective")
    bool bActive = false;

    UPROPERTY(VisibleInstanceOnly, Category="Objective")
    bool bCaptured = false;

    void UpdatePresentation();
};
