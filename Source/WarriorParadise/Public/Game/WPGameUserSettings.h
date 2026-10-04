#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "WPGameUserSettings.generated.h"

UENUM(BlueprintType)
enum class EWPGraphicsPreset : uint8
{
    Auto UMETA(DisplayName="Auto"),
    Minimal UMETA(DisplayName="Minimal / Integrated"),
    Low UMETA(DisplayName="Low"),
    Medium UMETA(DisplayName="Medium"),
    High UMETA(DisplayName="High"),
    Ultra UMETA(DisplayName="Ultra")
};

UCLASS(Config=GameUserSettings, ConfigDoNotCheckDefaults)
class WARRIORPARADISE_API UWPGameUserSettings : public UGameUserSettings
{
    GENERATED_BODY()

public:
    UWPGameUserSettings();

    static UWPGameUserSettings* Get();

    virtual void SetToDefaults() override;

    UFUNCTION(BlueprintCallable, Category="Settings|Graphics")
    void EnsureInitialHardwareBenchmark();

    UFUNCTION(BlueprintCallable, Category="Settings|Graphics")
    void RunAutoBenchmark();

    UFUNCTION(BlueprintCallable, Category="Settings|Graphics")
    void ApplyGraphicsPreset(
        EWPGraphicsPreset NewPreset,
        bool bSaveSettings = true);

    UFUNCTION(BlueprintCallable, Category="Settings|Graphics")
    EWPGraphicsPreset CycleGraphicsPreset();

    UFUNCTION(BlueprintPure, Category="Settings|Graphics")
    EWPGraphicsPreset GetGraphicsPreset() const
    {
        return GraphicsPreset;
    }

    UFUNCTION(BlueprintPure, Category="Settings|Graphics")
    FString GetGraphicsPresetLabel() const;

    UFUNCTION(BlueprintPure, Category="Settings|Graphics")
    bool HasRunInitialHardwareBenchmark() const
    {
        return bHasRunInitialHardwareBenchmark;
    }

private:
    UPROPERTY(Config)
    EWPGraphicsPreset GraphicsPreset = EWPGraphicsPreset::Auto;

    UPROPERTY(Config)
    bool bHasRunInitialHardwareBenchmark = false;

    UPROPERTY(Config)
    int32 WPSettingsVersion = 2;

    void ApplyManualQuality(
        int32 OverallQuality,
        float ResolutionScalePercent,
        float FrameRateLimit);

    void ApplyMinimalRenderingOverrides(bool bEnable);
};
