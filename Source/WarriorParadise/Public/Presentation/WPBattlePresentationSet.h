#pragma once

#include "CoreMinimal.h"
#include "Combat/WPCombatTypes.h"
#include "Engine/DataAsset.h"
#include "WPBattlePresentationSet.generated.h"

class UWPAirSupportVisualProfile;
class UWPEmplacementVisualProfile;
class UWPEnvironmentVisualProfile;
class UWPExplosionPresentationProfile;
class UWPInfantryLoadoutProfile;
class UWPLightVehicleVisualProfile;
class UWPSmokePresentationProfile;
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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Light Vehicles")
    TObjectPtr<UWPLightVehicleVisualProfile> AlliedTransport;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Light Vehicles")
    TObjectPtr<UWPLightVehicleVisualProfile> AlliedArmoredCar;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Light Vehicles")
    TObjectPtr<UWPLightVehicleVisualProfile> AxisArmoredCar;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Emplacements")
    TObjectPtr<UWPEmplacementVisualProfile> AlliedHMG;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Emplacements")
    TObjectPtr<UWPEmplacementVisualProfile> AlliedATGun;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Emplacements")
    TObjectPtr<UWPEmplacementVisualProfile> AxisHMG;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Emplacements")
    TObjectPtr<UWPEmplacementVisualProfile> AxisATGun;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aircraft")
    TObjectPtr<UWPAirSupportVisualProfile> AlliedAircraft;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aircraft")
    TObjectPtr<UWPAirSupportVisualProfile> AxisAircraft;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Environment")
    TObjectPtr<UWPEnvironmentVisualProfile> Environment;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosions")
    TObjectPtr<UWPExplosionPresentationProfile> FragExplosion;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosions")
    TObjectPtr<UWPExplosionPresentationProfile> ArtilleryExplosion;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Smoke")
    TObjectPtr<UWPSmokePresentationProfile> SmokePresentation;

    UWPInfantryLoadoutProfile* GetInfantryLoadout(
        EWPCombatTeam Team,
        EWPSoldierRole Role) const
    {
        const bool bAllies = Team == EWPCombatTeam::Allies;

        switch (Role)
        {
            case EWPSoldierRole::Assault:
                return bAllies ? AlliedAssault.Get() : AxisAssault.Get();
            case EWPSoldierRole::Medic:
                return bAllies ? AlliedMedic.Get() : AxisMedic.Get();
            case EWPSoldierRole::Engineer:
                return bAllies ? AlliedEngineer.Get() : AxisEngineer.Get();
            case EWPSoldierRole::Rifleman:
            default:
                return bAllies ? AlliedRifleman.Get() : AxisRifleman.Get();
        }
    }

    UWPVehicleVisualProfile* GetVehicleVisual(EWPCombatTeam Team) const
    {
        return Team == EWPCombatTeam::Allies
            ? AlliedVehicle.Get()
            : AxisVehicle.Get();
    }

    UWPLightVehicleVisualProfile* GetLightVehicleVisual(
        EWPCombatTeam Team,
        EWPLightVehicleType Type) const
    {
        if (Team == EWPCombatTeam::Allies)
        {
            return Type == EWPLightVehicleType::TransportTruck
                ? AlliedTransport.Get()
                : AlliedArmoredCar.Get();
        }

        return AxisArmoredCar.Get();
    }

    UWPEmplacementVisualProfile* GetEmplacementVisual(
        EWPCombatTeam Team,
        EWPEmplacementType Type) const
    {
        if (Team == EWPCombatTeam::Allies)
        {
            return Type == EWPEmplacementType::AntiTankGun
                ? AlliedATGun.Get()
                : AlliedHMG.Get();
        }

        return Type == EWPEmplacementType::AntiTankGun
            ? AxisATGun.Get()
            : AxisHMG.Get();
    }

    UWPAirSupportVisualProfile* GetAircraftVisual(EWPCombatTeam Team) const
    {
        return Team == EWPCombatTeam::Allies
            ? AlliedAircraft.Get()
            : AxisAircraft.Get();
    }

    UWPExplosionPresentationProfile* GetFragExplosion() const
    {
        return FragExplosion.Get();
    }

    UWPExplosionPresentationProfile* GetArtilleryExplosion() const
    {
        return ArtilleryExplosion.Get();
    }

    UWPSmokePresentationProfile* GetSmokePresentation() const
    {
        return SmokePresentation.Get();
    }
};
