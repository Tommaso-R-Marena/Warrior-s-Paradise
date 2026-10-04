#include "Presentation/WPWeaponPresentationComponent.h"
#include "Characters/WPSoldierCharacter.h"
#include "Vehicles/WPArmoredVehicle.h"
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

    RuntimeAttenuation = NewObject<USoundAttenuation>(
        this,
        TEXT("RuntimeOutdoorWeaponAttenuation"));

    if (RuntimeAttenuation)
    {
        FSoundAttenuationSettings& Settings =
            RuntimeAttenuation->Attenuation;

        Settings.bAttenuate = true;
        Settings.bSpatialize = true;
        Settings.AttenuationShape = EAttenuationShape::Sphere;
        Settings.AttenuationShapeExtents = FVector(300.0f);
        Settings.FalloffDistance = 11000.0f;

        Settings.bEnableOcclusion = true;
        Settings.OcclusionTraceChannel = ECC_Visibility;
        Settings.OcclusionLowPassFilterFrequency = 1800.0f;
        Settings.OcclusionVolumeAttenuation = 0.45f;
        Settings.OcclusionInterpolationTime = 0.08f;
    }

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

    if (!Attenuation)
    {
        Attenuation = RuntimeAttenuation;
    }

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

    enum class ELocalImpactSurface : uint8
    {
        Generic,
        Wood,
        Stone,
        Metal,
        Flesh
    };

    ELocalImpactSurface Surface = ELocalImpactSurface::Generic;

    if (HitActor)
    {
        if (HitActor->IsA<AWPSoldierCharacter>())
        {
            Surface = ELocalImpactSurface::Flesh;
        }
        else if (HitActor->IsA<AWPArmoredVehicle>() ||
                 HitActor->ActorHasTag(TEXT("WP_Surface_Metal")))
        {
            Surface = ELocalImpactSurface::Metal;
        }
        else if (HitActor->ActorHasTag(TEXT("WP_Surface_Wood")))
        {
            Surface = ELocalImpactSurface::Wood;
        }
        else if (HitActor->ActorHasTag(TEXT("WP_Surface_Stone")) ||
                 HitActor->ActorHasTag(TEXT("WP_Surface_Brick")) ||
                 HitActor->ActorHasTag(TEXT("WP_Surface_Plaster")) ||
                 HitActor->ActorHasTag(TEXT("WP_Surface_Concrete")))
        {
            Surface = ELocalImpactSurface::Stone;
        }
    }

    TSoftObjectPtr<UNiagaraSystem> Effect = PresentationProfile->BulletImpact;
    TSoftObjectPtr<USoundBase> ImpactSound =
        PresentationProfile->BulletImpactSound;

    switch (Surface)
    {
        case ELocalImpactSurface::Wood:
            if (!PresentationProfile->WoodImpact.IsNull())
            {
                Effect = PresentationProfile->WoodImpact;
            }
            if (!PresentationProfile->WoodImpactSound.IsNull())
            {
                ImpactSound = PresentationProfile->WoodImpactSound;
            }
            break;

        case ELocalImpactSurface::Stone:
            if (!PresentationProfile->StoneImpact.IsNull())
            {
                Effect = PresentationProfile->StoneImpact;
            }
            if (!PresentationProfile->StoneImpactSound.IsNull())
            {
                ImpactSound = PresentationProfile->StoneImpactSound;
            }
            break;

        case ELocalImpactSurface::Metal:
            if (!PresentationProfile->MetalImpact.IsNull())
            {
                Effect = PresentationProfile->MetalImpact;
            }
            if (!PresentationProfile->MetalImpactSound.IsNull())
            {
                ImpactSound = PresentationProfile->MetalImpactSound;
            }
            break;

        case ELocalImpactSurface::Flesh:
            if (!PresentationProfile->FleshImpact.IsNull())
            {
                Effect = PresentationProfile->FleshImpact;
            }
            if (!PresentationProfile->FleshImpactSound.IsNull())
            {
                ImpactSound = PresentationProfile->FleshImpactSound;
            }
            break;

        case ELocalImpactSurface::Generic:
        default:
            break;
    }

    if (UNiagaraSystem* System = Effect.LoadSynchronous())
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

    if (USoundBase* Sound = ImpactSound.LoadSynchronous())
    {
        PlaySound(
            Sound,
            ImpactLocation,
            PresentationProfile->ImpactVolume);
    }
}
