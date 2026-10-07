"""Source-level regression guards for smoke-assisted flanking.

These check ordering and target/waypoint constraints in the real C++ source.
They do not simulate UE gameplay or replace UHT, compilation or NullRHI.
"""
from __future__ import annotations

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AI = (ROOT / "Source/WarriorParadise/Private/AI/WPEnemySoldier.cpp").read_text(
    encoding="utf-8"
)
GAME_MODE = (
    ROOT / "Source/WarriorParadise/Private/Game/WPCombatGameMode.cpp"
).read_text(encoding="utf-8")


def function_body(start: str, next_function: str) -> str:
    first = AI.index(start)
    last = AI.index(next_function, first + len(start))
    return AI[first:last]


class SmokeManeuverContracts(unittest.TestCase):
    def test_last_grenade_can_still_authorize_existing_smoke(self) -> None:
        body = function_body(
            "bool AWPEnemySoldier::TryPrepareSmokeAssistedManeuver(",
            "bool AWPEnemySoldier::IsSmokeAssistedManeuverReady(",
        )
        self.assertLess(
            body.index("IsSmokeAssistedManeuverReady(Time)"),
            body.index("GetSmokeGrenadesRemaining() <= 0"),
        )

    def test_flank_waypoint_is_frozen_at_throw_time(self) -> None:
        body = function_body(
            "bool AWPEnemySoldier::TryPrepareSmokeAssistedManeuver(",
            "bool AWPEnemySoldier::IsSmokeAssistedManeuverReady(",
        )
        self.assertLess(
            body.index("GetManeuverDestination(Target)"),
            body.index("ThrowSmokeGrenadeAt(SmokeTarget)"),
        )
        self.assertIn("ManeuverSmokeFlankDestination = PlannedFlank;", body)

    def test_real_smoke_is_required(self) -> None:
        body = function_body(
            "bool AWPEnemySoldier::IsSmokeAssistedManeuverReady(",
            "float AWPEnemySoldier::GetManeuverSelectionScore(",
        )
        self.assertIn("TActorIterator<AWPSmokeCloud>", body)
        self.assertIn("GetSmokeRadiusCm()", body)
        self.assertIn("ManeuverSmokeTargetLocation", body)
        self.assertIn("ManeuverSmokeExpireTime", body)

    def test_target_retention_is_contact_bound_and_time_bounded(self) -> None:
        tick = AI[AI.index("void AWPEnemySoldier::Tick(float DeltaSeconds)"):]
        self.assertIn("const bool bRetainSmokeContact", tick)
        self.assertIn("SmokeManeuverTarget.Get() ==", tick)
        self.assertIn("CurrentTarget.Get()", tick)
        self.assertIn("ManeuverSmokeExpireTime > Time", tick)
        self.assertIn("!bRetainSmokeContact", tick)

    def test_hidden_move_uses_snapshot_not_live_target_location(self) -> None:
        tick = AI[AI.index("void AWPEnemySoldier::Tick(float DeltaSeconds)"):]
        start = tick.index("if (!bTargetVisible &&\n        bSmokeManeuverWindowActive)")
        end = tick.index("const FVector ToTarget =", start)
        concealed = tick[start:end]
        self.assertIn("MoveTacticallyToward(", concealed)
        self.assertIn("ManeuverSmokeFlankDestination", concealed)
        self.assertNotIn("Target->GetActorLocation()", concealed)
        self.assertIn("bHasAdvancedUnderManeuverSmoke = true;", concealed)

    def test_actual_advance_has_separate_headless_counter(self) -> None:
        self.assertIn("ManeuverSmokeUses", GAME_MODE)
        self.assertIn("ManeuverSmokeAdvances", GAME_MODE)
        self.assertIn("smokeManeuverAdvances=%d", GAME_MODE)


if __name__ == "__main__":
    unittest.main()
