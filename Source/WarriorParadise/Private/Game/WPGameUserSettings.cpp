#include "Game/WPGameUserSettings.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

namespace
{
constexpr int32 CurrentWPSettingsVersion = 2;
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
#if PLATFORM_ANDROID || PLATFORM_IOS
    // Native phone launches default conservatively; GPU tier selection is
    // governed by Android/iOS device profiles, not a desktop GPU benchmark.
    SetFrameRateLimit(30.0f);
#else
    SetFrameRateLimit(60.0f);
    SetFullscreenMode(EWindowMode::WindowedFullscreen);
#endif
}

void UWPGameUserSettings::EnsureInitialHardwareBenchmark()
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    // Mobile GPUs are selected through renderer device profiles and should
    // never execute the PC hardware benchmark on first launch.
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
    else if (GraphicsPreset != EWPGraphicsPreset::Auto)
    {
        ApplyGraphicsPreset(GraphicsPreset, false);
    }
    return;
#endif

    if (WPSettingsVersion != CurrentWPSettingsVersion)
    {
        WPSettingsVersion = CurrentWPSettingsVersion;
        bHasRunInitialHardwareBenchmark = false;
        GraphicsPreset = EWPGraphicsPreset::Auto;
    }

    if (!bHasRunInitialHardwareBenchmark)
    {
        RunAutoBenchmark();
        return;
    }

    // Custom render-feature overrides (especially Minimal) are runtime
    // console-variable state, so re-apply a saved manual preset after
    // settings have been loaded on a new process.
    if (GraphicsPreset != EWPGraphicsPreset::Auto)
    {
        ApplyGraphicsPreset(GraphicsPreset, false);
    }
}

void UWPGameUserSettings::RunAutoBenchmark()
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    // Respect Android_Low/Mid/High and GPU-family overrides. Avoid
    // RunHardwareBenchmark and desktop-specific Lumen/VSM toggles.
    SetFrameRateLimit(30.0f);
    SetDynamicResolutionEnabled(false);
    GraphicsPreset = EWPGraphicsPreset::Auto;
    bHasRunInitialHardwareBenchmark = true;
    WPSettingsVersion = CurrentWPSettingsVersion;
    ApplySettings(false);
    SaveSettings();
    return;
#endif

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

void UWPGameUserSettings::ApplyMinimalRenderingOverrides(bool bEnable)
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    // Desktop feature CVar defaults must not override Mobile Forward.
    return;
#endif

    struct FCVarSetting
    {
        const TCHAR* Name;
        int32 MinimalValue;
        int32 NormalValue;
    };

    static const FCVarSetting Settings[] =
    {
        { TEXT("r.DynamicGlobalIlluminationMethod"), 0, 1 },
        { TEXT("r.ReflectionMethod"), 0, 1 },
        { TEXT("r.Shadow.Virtual.Enable"), 0, 1 },
        { TEXT("r.VolumetricCloud"), 0, 1 },
        { TEXT("r.AntiAliasingMethod"), 1, 4 }
    };

    for (const FCVarSetting& Setting : Settings)
    {
        if (IConsoleVariable* Variable =
            IConsoleManager::Get().FindConsoleVariable(Setting.Name))
        {
            Variable->Set(
                bEnable ? Setting.MinimalValue : Setting.NormalValue,
                ECVF_SetByGameOverride,
                NAME_None);
        }
    }
}

void UWPGameUserSettings::ApplyGraphicsPreset(
    EWPGraphicsPreset NewPreset,
    bool bSaveSettings)
{
    if (NewPreset == EWPGraphicsPreset::Auto)
    {
        ApplyMinimalRenderingOverrides(false);
        RunAutoBenchmark();
        return;
    }

    ApplyMinimalRenderingOverrides(
        NewPreset == EWPGraphicsPreset::Minimal);

    switch (NewPreset)
    {
        case EWPGraphicsPreset::Minimal:
            ApplyManualQuality(0, 50.0f, 30.0f);
            break;

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
    EWPGraphicsPreset NextPreset = EWPGraphicsPreset::Minimal;

    switch (GraphicsPreset)
    {
        case EWPGraphicsPreset::Minimal:
            NextPreset = EWPGraphicsPreset::Low;
            break;

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
            NextPreset = EWPGraphicsPreset::Minimal;
            break;
    }

    ApplyGraphicsPreset(NextPreset, true);
    return NextPreset;
}

FString UWPGameUserSettings::GetGraphicsPresetLabel() const
{
    switch (GraphicsPreset)
    {
        case EWPGraphicsPreset::Minimal:
            return TEXT("MINIMAL");

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
