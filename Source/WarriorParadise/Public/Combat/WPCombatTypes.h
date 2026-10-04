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
