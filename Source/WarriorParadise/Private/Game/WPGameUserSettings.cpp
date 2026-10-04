#include "Game/WPGameUserSettings.h"
#include "Engine/Engine.h"

namespace
{
constexpr int32 CurrentWPSettingsVersion = 1;
}

UWPGameUserSettings::UWPGameUserSettings()
{
    WPSettingsVersion = CurrentWPSettingsVersion;
}

UWPGameUserSettings* UWPGameUserSettings::Get()
{
    if (!GEngine)
    {
        return nullptr;
    }

    return Cast<UWPGameUserSettings>(
        GEngine->GetGameUserSettings());
}

void UWPGameUserSettings::SetToDefaults()
{
    Super::SetToDefaults();

    GraphicsPreset = EWPGraphicsPreset::Auto;
    bHasRunInitialHardwareBenchmark = false;
    WPSettingsVersion = CurrentWPSettingsVersion;

    SetVSyncEnabled(false);
    SetFrameRateLimit(60.0f);
    SetFullscreenMode(EWindowMode::WindowedFullscreen);
}

void UWPGameUserSettings::EnsureInitialHardwareBenchmark()
{
    if (WPSettingsVersion != CurrentWPSettingsVersion)
    {
        WPSettingsVersion = CurrentWPSettingsVersion;
        bHasRunInitialHardwareBenchmark = false;
        GraphicsPreset = EWPGraphicsPreset::Auto;
    }

    if (!bHasRunInitialHardwareBenchmark)
    {
        RunAutoBenchmark();
    }
}

void UWPGameUserSettings::RunAutoBenchmark()
{
    // UE's benchmark populates per-group scalability values from the
    // project's PerfIndexThresholds. Keep it deliberately mixed rather
    // than forcing every group to one level.
    RunHardwareBenchmark(10, 1.0f, 1.0f);

    // Do not let an unusually aggressive benchmark result render below
    // the minimum presentation target for the PC vertical slice.
    ScalabilityQuality.ResolutionQuality = FMath::Clamp(
        ScalabilityQuality.ResolutionQuality,
        67.0f,
        100.0f);

    ApplyHardwareBenchmarkResults();

    GraphicsPreset = EWPGraphicsPreset::Auto;
    bHasRunInitialHardwareBenchmark = true;
    WPSettingsVersion = CurrentWPSettingsVersion;
    SaveSettings();
}

void UWPGameUserSettings::ApplyManualQuality(
    int32 OverallQuality,
    float ResolutionScalePercent,
    float NewFrameRateLimit)
{
    SetOverallScalabilityLevel(OverallQuality);
    SetResolutionScaleValueEx(ResolutionScalePercent);
    SetFrameRateLimit(NewFrameRateLimit);

    // Keep manual presets deterministic. Resolution scaling is explicit,
    // rather than allowing an unrelated dynamic-resolution policy to
    // override the selected quality tier.
    SetDynamicResolutionEnabled(false);
}

void UWPGameUserSettings::ApplyGraphicsPreset(
    EWPGraphicsPreset NewPreset,
    bool bSaveSettings)
{
    if (NewPreset == EWPGraphicsPreset::Auto)
    {
        RunAutoBenchmark();
        return;
    }

    switch (NewPreset)
    {
        case EWPGraphicsPreset::Low:
            ApplyManualQuality(0, 67.0f, 60.0f);
            break;

        case EWPGraphicsPreset::Medium:
            ApplyManualQuality(1, 78.0f, 60.0f);
            break;

        case EWPGraphicsPreset::High:
            ApplyManualQuality(2, 90.0f, 60.0f);
            break;

        case EWPGraphicsPreset::Ultra:
            ApplyManualQuality(3, 100.0f, 120.0f);
            break;

        case EWPGraphicsPreset::Auto:
        default:
            break;
    }

    GraphicsPreset = NewPreset;
    bHasRunInitialHardwareBenchmark = true;
    WPSettingsVersion = CurrentWPSettingsVersion;

    ApplySettings(false);

    if (bSaveSettings)
    {
        SaveSettings();
    }
}

EWPGraphicsPreset UWPGameUserSettings::CycleGraphicsPreset()
{
    EWPGraphicsPreset NextPreset = EWPGraphicsPreset::Low;

    switch (GraphicsPreset)
    {
        case EWPGraphicsPreset::Low:
            NextPreset = EWPGraphicsPreset::Medium;
            break;

        case EWPGraphicsPreset::Medium:
            NextPreset = EWPGraphicsPreset::High;
            break;

        case EWPGraphicsPreset::High:
            NextPreset = EWPGraphicsPreset::Ultra;
            break;

        case EWPGraphicsPreset::Ultra:
        case EWPGraphicsPreset::Auto:
        default:
            NextPreset = EWPGraphicsPreset::Low;
            break;
    }

    ApplyGraphicsPreset(NextPreset, true);
    return NextPreset;
}

FString UWPGameUserSettings::GetGraphicsPresetLabel() const
{
    switch (GraphicsPreset)
    {
        case EWPGraphicsPreset::Low:
            return TEXT("LOW");

        case EWPGraphicsPreset::Medium:
            return TEXT("MEDIUM");

        case EWPGraphicsPreset::High:
            return TEXT("HIGH");

        case EWPGraphicsPreset::Ultra:
            return TEXT("ULTRA");

        case EWPGraphicsPreset::Auto:
        default:
            return TEXT("AUTO");
    }
}
