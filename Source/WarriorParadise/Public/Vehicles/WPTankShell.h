#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "GameFramework/Actor.h"
#include "WPTankShell.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FWPTankShellImpactSignature,
    FVector,
    ImpactLocation);

UCLASS()
class WARRIORPARADISE_API AWPTankShell : public AActor
{
    GENERATED_BODY()

public:
    AWPTankShell();

    UPROPERTY(BlueprintAssignable, Category="Shell|Presentation")
    FWPTankShellImpactSignature OnImpact;

    void InitializeShell(
        EWPCombatTeam InTeam,
        const FVector& Velocity,
        EWPArmorShellType InShellType = EWPArmorShellType::ArmorPiercing);

    UFUNCTION(BlueprintPure, Category="Shell")
    EWPArmorShellType GetShellType() const { return ShellType; }

private:
    UPROPERTY(VisibleAnywhere, Category="Shell")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category="Shell")
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY(EditDefaultsOnly, Category="Shell|AP")
    float APMaximumDamage = 180.0f;

    UPROPERTY(EditDefaultsOnly, Category="Shell|AP")
    float APDirectArmorDamage = 560.0f;

    UPROPERTY(EditDefaultsOnly, Category="Shell|AP")
    float APBlastRadiusCm = 360.0f;

    UPROPERTY(EditDefaultsOnly, Category="Shell|HE")
    float HEMaximumDamage = 390.0f;

    UPROPERTY(EditDefaultsOnly, Category="Shell|HE")
    float HEDirectArmorDamage = 220.0f;

    UPROPERTY(EditDefaultsOnly, Category="Shell|HE")
    float HEBlastRadiusCm = 760.0f;

    UPROPERTY(EditDefaultsOnly, Category="Shell")
    float SuppressionRadiusCm = 1250.0f;

    float MaximumDamage = 180.0f;
    float DirectArmorDamage = 560.0f;
    float BlastRadiusCm = 360.0f;

    EWPCombatTeam ShellTeam = EWPCombatTeam::Allies;
    EWPArmorShellType ShellType = EWPArmorShellType::ArmorPiercing;

    UFUNCTION()
    void HandleHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse,
        const FHitResult& Hit);

    void Explode(const FVector& ImpactLocation, AActor* DirectHitActor = nullptr);
};
