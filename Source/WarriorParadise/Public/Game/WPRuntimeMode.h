#pragma once

#include "CoreMinimal.h"

namespace WPRuntimeMode
{
    WARRIORPARADISE_API bool IsHeadlessSimulation();
    WARRIORPARADISE_API bool IsHeadlessSmokeTest();
    WARRIORPARADISE_API bool WantsIntegratedGraphics();
    WARRIORPARADISE_API float GetHeadlessSmokeDurationSeconds(
        float DefaultSeconds = 15.0f);
}
