#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "Engine/DataAsset.h"
#include "WPBattlePresentationSet.generated.h"

class UWPEnvironmentVisualProfile;
class UWPInfantryLoadoutProfile;
class UWPVehicleVisualProfile;

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPBattlePresentationSet : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Allies")
    TObjectPtr<UWPInfantryLoadoutProfile> AlliedRifleman;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Allies")
    TObjectPtr<UWPInfantryLoadoutProfile> AlliedAssault;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Allies")
    TObjectPtr<UWPInfantryLoadoutProfile> AlliedMedic;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Allies")
    TObjectPtr<UWPInfantryLoadoutProfile> AlliedEngineer;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Axis")
    TObjectPtr<UWPInfantryLoadoutProfile> AxisRifleman;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Axis")
    TObjectPtr<UWPInfantryLoadoutProfile> AxisAssault;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Axis")
    TObjectPtr<UWPInfantryLoadoutProfile> AxisMedic;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Axis")
    TObjectPtr<UWPInfantryLoadoutProfile> AxisEngineer;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicles")
    TObjectPtr<UWPVehicleVisualProfile> AlliedVehicle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicles")
    TObjectPtr<UWPVehicleVisualProfile> AxisVehicle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Environment")
    TObjectPtr<UWPEnvironmentVisualProfile> Environment;

    UWPInfantryLoadoutProfile* GetInfantryLoadout(
        EWPCombatTeam Team,
        EWPSoldierRole Role) const
    {
        const bool bAllies = Team == EWPCombatTeam::Allies;

        switch (Role)
        {
            case EWPSoldierRole::Assault:
                return bAllies ? AlliedAssault : AxisAssault;
            case EWPSoldierRole::Medic:
                return bAllies ? AlliedMedic : AxisMedic;
            case EWPSoldierRole::Engineer:
                return bAllies ? AlliedEngineer : AxisEngineer;
            case EWPSoldierRole::Rifleman:
            default:
                return bAllies ? AlliedRifleman : AxisRifleman;
        }
    }

    UWPVehicleVisualProfile* GetVehicleVisual(EWPCombatTeam Team) const
    {
        return Team == EWPCombatTeam::Allies
            ? AlliedVehicle
            : AxisVehicle;
    }
};
