#pragma once

#include "CoreMinimal.h"
#include "WPCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EWPCombatTeam : uint8
{
    Allies UMETA(DisplayName="Allies"),
    Axis UMETA(DisplayName="Axis")
};

UENUM(BlueprintType)
enum class EWPSoldierRole : uint8
{
    Rifleman UMETA(DisplayName="Rifleman"),
    Assault UMETA(DisplayName="Assault"),
    Medic UMETA(DisplayName="Medic"),
    Engineer UMETA(DisplayName="Engineer")
};

UENUM(BlueprintType)
enum class EWPSquadOrder : uint8
{
    Follow UMETA(DisplayName="Follow"),
    AttackObjective UMETA(DisplayName="Attack Objective"),
    HoldPosition UMETA(DisplayName="Hold Position")
};

UENUM(BlueprintType)
enum class EWPArmorShellType : uint8
{
    ArmorPiercing UMETA(DisplayName="Armor Piercing"),
    HighExplosive UMETA(DisplayName="High Explosive")
};

UENUM(BlueprintType)
enum class EWPVehicleSubsystem : uint8
{
    Engine UMETA(DisplayName="Engine"),
    Tracks UMETA(DisplayName="Tracks"),
    Turret UMETA(DisplayName="Turret"),
    AmmoRack UMETA(DisplayName="Ammo Rack")
};

UENUM(BlueprintType)
enum class EWPLightVehicleType : uint8
{
    TransportTruck UMETA(DisplayName="Transport Truck"),
    ArmoredCar UMETA(DisplayName="Armored Car")
};

UENUM(BlueprintType)
enum class EWPEmplacementType : uint8
{
    HeavyMachineGun UMETA(DisplayName="Heavy Machine Gun"),
    AntiTankGun UMETA(DisplayName="Anti-Tank Gun")
};

UENUM(BlueprintType)
enum class EWPAirSupportMission : uint8
{
    Flyby UMETA(DisplayName="Flyby"),
    Strafe UMETA(DisplayName="Strafe"),
    Bombing UMETA(DisplayName="Bombing")
};
