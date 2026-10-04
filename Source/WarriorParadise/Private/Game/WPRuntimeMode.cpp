#include "Game/WPRuntimeMode.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
bool HasFlag(const TCHAR* Name)
{
    return FParse::Param(FCommandLine::Get(), Name);
}
}

bool WPRuntimeMode::IsHeadlessSmokeTest()
{
    return HasFlag(TEXT("WPHeadlessSmoke"));
}

bool WPRuntimeMode::IsHeadlessSimulation()
{
    return
        IsHeadlessSmokeTest() ||
        HasFlag(TEXT("WPHeadless")) ||
        HasFlag(TEXT("nullrhi"));
}

bool WPRuntimeMode::WantsIntegratedGraphics()
{
    return HasFlag(TEXT("WPIntegratedGraphics"));
}

float WPRuntimeMode::GetHeadlessSmokeDurationSeconds(
    float DefaultSeconds)
{
    float Seconds = DefaultSeconds;
    FParse::Value(
        FCommandLine::Get(),
        TEXT("WPSmokeSeconds="),
        Seconds);

    return FMath::Clamp(Seconds, 5.0f, 300.0f);
}
