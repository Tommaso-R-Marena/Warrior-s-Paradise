#pragma once

#include "CoreMinimal.h"
#include "WPCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EWPCombatTeam : uint8
{
    Allies UMETA(DisplayName="Allies"),
    Axis UMETA(DisplayName="Axis")
};
