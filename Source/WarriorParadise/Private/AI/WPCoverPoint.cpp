#include "AI/WPCoverPoint.h"
#include "Characters/WPSoldierCharacter.h"

AWPCoverPoint::AWPCoverPoint()
{
    PrimaryActorTick.bCanEverTick = false;
    SetActorEnableCollision(false);
}

void AWPCoverPoint::InitializeCover(const FVector& InCoverNormal, float InQuality)
{
    CoverNormal = InCoverNormal.GetSafeNormal2D();
    if (CoverNormal.IsNearlyZero())
    {
        CoverNormal = FVector::ForwardVector;
    }

    CoverQuality = FMath::Clamp(InQuality, 0.1f, 2.0f);
}

bool AWPCoverPoint::IsAvailableFor(const AWPSoldierCharacter* Soldier) const
{
    return !Occupant.IsValid() || Occupant.Get() == Soldier;
}

bool AWPCoverPoint::TryReserve(AWPSoldierCharacter* Soldier)
{
    if (!Soldier || !IsAvailableFor(Soldier))
    {
        return false;
    }

    Occupant = Soldier;
    return true;
}

void AWPCoverPoint::Release(AWPSoldierCharacter* Soldier)
{
    if (Occupant.Get() == Soldier)
    {
        Occupant.Reset();
    }
}
