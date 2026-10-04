#include "Presentation/WPSoldierPresentationComponent.h"
#include "Animation/AnimMontage.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/WPSoldierPresentationProfile.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

UWPSoldierPresentationComponent::UWPSoldierPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWPSoldierPresentationComponent::BeginPlay()
{
    Super::BeginPlay();

    SoldierOwner = Cast<AWPSoldierCharacter>(GetOwner());
    HealthComponent = SoldierOwner.IsValid()
        ? SoldierOwner->GetHealthComponent()
        : nullptr;

    RuntimeAttenuation = NewObject<USoundAttenuation>(
        this,
        TEXT("RuntimeSoldierAttenuation"));

    if (RuntimeAttenuation)
    {
        FSoundAttenuationSettings& Settings =
            RuntimeAttenuation->Attenuation;

        Settings.bAttenuate = true;
        Settings.bSpatialize = true;
        Settings.AttenuationShape = EAttenuationShape::Sphere;
        Settings.AttenuationShapeExtents = FVector(150.0f);
        Settings.FalloffDistance = 4200.0f;
        Settings.bEnableOcclusion = true;
        Settings.OcclusionTraceChannel = ECC_Visibility;
        Settings.OcclusionLowPassFilterFrequency = 2200.0f;
        Settings.OcclusionVolumeAttenuation = 0.55f;
        Settings.OcclusionInterpolationTime = 0.08f;
    }

    BindEvents();
}

void UWPSoldierPresentationComponent::SetPresentationProfile(
    UWPSoldierPresentationProfile* NewProfile)
{
    PresentationProfile = NewProfile;
}

void UWPSoldierPresentationComponent::BindEvents()
{
    AWPSoldierCharacter* Soldier = SoldierOwner.Get();
    UWPHealthComponent* Health = HealthComponent.Get();

    if (!Soldier || !Health)
    {
        return;
    }

    Health->OnDamageReceived.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleDamageReceived);
    Health->OnDowned.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleDowned);
    Health->OnRevived.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleRevived);
    Health->OnDeath.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleDeath);

    Soldier->OnMeleeAttack.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleMelee);
    Soldier->OnBandageStarted.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleBandageStarted);
    Soldier->OnBandageCompleted.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleBandageCompleted);
}

void UWPSoldierPresentationComponent::PlayWorldSound(
    USoundBase* Sound,
    float VolumeMultiplier) const
{
    if (!Sound || !PresentationProfile || !SoldierOwner.IsValid())
    {
        return;
    }

    USoundAttenuation* Attenuation =
        PresentationProfile->Attenuation.LoadSynchronous();

    if (!Attenuation)
    {
        Attenuation = RuntimeAttenuation;
    }

    const float Pitch = 1.0f + FMath::FRandRange(
        -PresentationProfile->PitchRandomization,
        PresentationProfile->PitchRandomization);

    UGameplayStatics::PlaySoundAtLocation(
        this,
        Sound,
        SoldierOwner->GetActorLocation(),
        PresentationProfile->Volume * VolumeMultiplier,
        Pitch,
        0.0f,
        Attenuation,
        nullptr,
        nullptr);
}

void UWPSoldierPresentationComponent::PlayMontage(
    UAnimMontage* Montage) const
{
    if (!Montage || !SoldierOwner.IsValid())
    {
        return;
    }

    SoldierOwner->PlayAnimMontage(Montage);
}

void UWPSoldierPresentationComponent::SpawnEffectAtSoldier(
    UNiagaraSystem* System,
    const FVector& Scale) const
{
    if (!System || !SoldierOwner.IsValid())
    {
        return;
    }

    const FVector Location =
        SoldierOwner->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        this,
        System,
        Location,
        SoldierOwner->GetActorRotation(),
        Scale,
        true,
        true,
        ENCPoolMethod::AutoRelease,
        true);
}

void UWPSoldierPresentationComponent::HandleDamageReceived(
    float Damage,
    AActor* DamageCauser)
{
    if (!PresentationProfile || Damage <= 0.0f)
    {
        return;
    }

    if (UNiagaraSystem* System =
        PresentationProfile->HitEffect.LoadSynchronous())
    {
        SpawnEffectAtSoldier(
            System,
            PresentationProfile->HitEffectScale);
    }

    if (UAnimMontage* Montage =
        PresentationProfile->HitReactMontage.LoadSynchronous())
    {
        PlayMontage(Montage);
    }

    if (!GetWorld() ||
        GetWorld()->GetTimeSeconds() < NextPainSoundTime)
    {
        return;
    }

    if (USoundBase* Sound =
        PresentationProfile->PainSound.LoadSynchronous())
    {
        PlayWorldSound(Sound);
        NextPainSoundTime =
            GetWorld()->GetTimeSeconds() +
            PresentationProfile->PainSoundCooldown;
    }
}

void UWPSoldierPresentationComponent::HandleDowned()
{
    if (!PresentationProfile)
    {
        return;
    }

    PlayWorldSound(
        PresentationProfile->DownedSound.LoadSynchronous());

    PlayMontage(
        PresentationProfile->DownedMontage.LoadSynchronous());
}

void UWPSoldierPresentationComponent::HandleRevived()
{
    if (!PresentationProfile)
    {
        return;
    }

    PlayWorldSound(
        PresentationProfile->ReviveSound.LoadSynchronous(),
        0.82f);

    PlayMontage(
        PresentationProfile->ReviveMontage.LoadSynchronous());
}

void UWPSoldierPresentationComponent::HandleDeath()
{
    if (!PresentationProfile)
    {
        return;
    }

    if (UNiagaraSystem* System =
        PresentationProfile->DeathEffect.LoadSynchronous())
    {
        SpawnEffectAtSoldier(
            System,
            PresentationProfile->DeathEffectScale);
    }

    PlayWorldSound(
        PresentationProfile->DeathSound.LoadSynchronous());

    PlayMontage(
        PresentationProfile->DeathMontage.LoadSynchronous());
}

void UWPSoldierPresentationComponent::HandleMelee()
{
    if (!PresentationProfile)
    {
        return;
    }

    PlayWorldSound(
        PresentationProfile->MeleeSound.LoadSynchronous(),
        0.75f);

    PlayMontage(
        PresentationProfile->MeleeMontage.LoadSynchronous());
}

void UWPSoldierPresentationComponent::HandleBandageStarted()
{
    if (!PresentationProfile)
    {
        return;
    }

    PlayWorldSound(
        PresentationProfile->BandageStartSound.LoadSynchronous(),
        0.55f);

    PlayMontage(
        PresentationProfile->BandageMontage.LoadSynchronous());
}

void UWPSoldierPresentationComponent::HandleBandageCompleted()
{
    if (!PresentationProfile)
    {
        return;
    }

    PlayWorldSound(
        PresentationProfile->BandageCompleteSound.LoadSynchronous(),
        0.55f);
}
