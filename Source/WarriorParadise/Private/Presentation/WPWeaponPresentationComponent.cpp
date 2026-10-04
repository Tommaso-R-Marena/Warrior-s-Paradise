#include "Presentation/WPWeaponPresentationComponent.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPBallisticProjectile.h"
#include "Combat/WPWeaponComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/WPWeaponPresentationProfile.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

UWPWeaponPresentationComponent::UWPWeaponPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWPWeaponPresentationComponent::BeginPlay()
{
    Super::BeginPlay();

    SoldierOwner = Cast<AWPSoldierCharacter>(GetOwner());
    BindWeapon();
}

void UWPWeaponPresentationComponent::SetPresentationProfile(
    UWPWeaponPresentationProfile* NewProfile)
{
    PresentationProfile = NewProfile;
}

void UWPWeaponPresentationComponent::BindWeapon()
{
    AWPSoldierCharacter* Soldier = SoldierOwner.Get();
    if (!Soldier)
    {
        return;
    }

    UWPWeaponComponent* Weapon = Soldier->GetWeaponComponent();
    if (!Weapon)
    {
        return;
    }

    WeaponComponent = Weapon;

    Weapon->OnShotFired.AddUniqueDynamic(
        this,
        &UWPWeaponPresentationComponent::HandleShotFired);

    Weapon->OnReloadStarted.AddUniqueDynamic(
        this,
        &UWPWeaponPresentationComponent::HandleReloadStarted);

    Weapon->OnReloadCompleted.AddUniqueDynamic(
        this,
        &UWPWeaponPresentationComponent::HandleReloadCompleted);

    Weapon->OnReloadCancelled.AddUniqueDynamic(
        this,
        &UWPWeaponPresentationComponent::HandleReloadCancelled);

    Weapon->OnDryFire.AddUniqueDynamic(
        this,
        &UWPWeaponPresentationComponent::HandleDryFire);

    Weapon->OnProjectileSpawned.AddUniqueDynamic(
        this,
        &UWPWeaponPresentationComponent::HandleProjectileSpawned);
}

void UWPWeaponPresentationComponent::PlaySound(
    USoundBase* Sound,
    const FVector& Location,
    float VolumeMultiplier) const
{
    if (!Sound || !PresentationProfile)
    {
        return;
    }

    USoundAttenuation* Attenuation =
        PresentationProfile->Attenuation.LoadSynchronous();

    const float RandomPitch = 1.0f + FMath::FRandRange(
        -PresentationProfile->PitchRandomization,
        PresentationProfile->PitchRandomization);

    UGameplayStatics::PlaySoundAtLocation(
        this,
        Sound,
        Location,
        VolumeMultiplier,
        RandomPitch,
        0.0f,
        Attenuation,
        nullptr,
        nullptr);
}

void UWPWeaponPresentationComponent::HandleShotFired(
    float RecoilPitch,
    float RecoilYaw)
{
    if (!PresentationProfile)
    {
        return;
    }

    AWPSoldierCharacter* Soldier = SoldierOwner.Get();
    if (!Soldier)
    {
        return;
    }

    const FTransform Muzzle = Soldier->GetWeaponPresentationTransform();

    if (UNiagaraSystem* System = PresentationProfile->MuzzleFlash.LoadSynchronous())
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            System,
            Muzzle.GetLocation(),
            Muzzle.Rotator(),
            FVector::OneVector,
            true,
            true,
            ENCPoolMethod::AutoRelease,
            true);
    }

    if (USoundBase* Sound = PresentationProfile->FireSound.LoadSynchronous())
    {
        PlaySound(Sound, Muzzle.GetLocation(), PresentationProfile->FireVolume);
    }
}

void UWPWeaponPresentationComponent::HandleReloadStarted()
{
    if (!PresentationProfile)
    {
        return;
    }

    if (USoundBase* Sound = PresentationProfile->ReloadStartSound.LoadSynchronous())
    {
        PlaySound(Sound, GetOwner()->GetActorLocation(), 1.0f);
    }
}

void UWPWeaponPresentationComponent::HandleReloadCompleted()
{
    if (!PresentationProfile)
    {
        return;
    }

    if (USoundBase* Sound = PresentationProfile->ReloadCompleteSound.LoadSynchronous())
    {
        PlaySound(Sound, GetOwner()->GetActorLocation(), 1.0f);
    }
}

void UWPWeaponPresentationComponent::HandleReloadCancelled()
{
    if (!PresentationProfile)
    {
        return;
    }

    if (USoundBase* Sound = PresentationProfile->ReloadCancelSound.LoadSynchronous())
    {
        PlaySound(Sound, GetOwner()->GetActorLocation(), 1.0f);
    }
}

void UWPWeaponPresentationComponent::HandleDryFire()
{
    if (!PresentationProfile)
    {
        return;
    }

    if (USoundBase* Sound = PresentationProfile->DryFireSound.LoadSynchronous())
    {
        const FVector Location = SoldierOwner.IsValid()
            ? SoldierOwner->GetWeaponPresentationTransform().GetLocation()
            : GetOwner()->GetActorLocation();

        PlaySound(Sound, Location, 1.0f);
    }
}

void UWPWeaponPresentationComponent::HandleProjectileSpawned(
    AWPBallisticProjectile* Projectile)
{
    if (!Projectile)
    {
        return;
    }

    Projectile->OnImpact.AddUniqueDynamic(
        this,
        &UWPWeaponPresentationComponent::HandleProjectileImpact);
}

void UWPWeaponPresentationComponent::HandleProjectileImpact(
    FVector ImpactLocation,
    FVector ImpactNormal,
    AActor* HitActor)
{
    if (!PresentationProfile)
    {
        return;
    }

    if (UNiagaraSystem* System = PresentationProfile->BulletImpact.LoadSynchronous())
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            System,
            ImpactLocation,
            ImpactNormal.Rotation(),
            FVector::OneVector,
            true,
            true,
            ENCPoolMethod::AutoRelease,
            true);
    }

    if (USoundBase* Sound = PresentationProfile->BulletImpactSound.LoadSynchronous())
    {
        PlaySound(
            Sound,
            ImpactLocation,
            PresentationProfile->ImpactVolume);
    }
}
