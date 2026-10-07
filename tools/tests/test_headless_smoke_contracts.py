"""Source guards for headless gameplay smoke in AI-only Unreal runs.

These guard fallback control flow and do not substitute for running NullRHI.
"""
from __future__ import annotations

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CPP = (
    ROOT / "Source/WarriorParadise/Private/Game/WPCombatGameMode.cpp"
).read_text(encoding="utf-8")
START = CPP.split(
    "void AWPCombatGameMode::StartHeadlessSmokeTest()", 1
)[1].split("void AWPCombatGameMode::FinishHeadlessSmokeTest()", 1)[0]


class HeadlessSmokeContracts(unittest.TestCase):
    def test_initial_squad_size_supports_no_player(self) -> None:
        self.assertIn("const int32 ExpectedFriendlies =", START)
        self.assertIn("TestSoldier ? 6 : 5", START)
        self.assertIn("ExpectedFriendlies", START)

    def test_health_and_suppression_subject_fallback(self) -> None:
        self.assertIn("TActorIterator<AWPSoldierCharacter>", START)
        self.assertIn("Candidate->IsCombatCapable()", START)
        self.assertIn("EWPCombatTeam::Allies", START)
        self.assertIn("TestSoldier = Candidate;", START)
        self.assertIn("TestSoldier->GetSuppressionComponent()", START)
        self.assertIn("TestSoldier->GetHealthComponent()", START)

    def test_real_damage_not_mocked(self) -> None:
        self.assertIn("UGameplayStatics::ApplyDamage(", START)
        self.assertIn("UDamageType::StaticClass()", START)
        self.assertIn("Health->IsBleeding()", START)
        self.assertIn("Health->StopBleeding()", START)

    def test_headless_logs_subject_selection(self) -> None:
        self.assertIn('TEXT("alliedAI")', START)
        self.assertIn('TEXT("player")', START)
        self.assertIn('TEXT("missing")', START)
        self.assertIn('subject=%s', START)

    def test_health_and_suppression_remain_required(self) -> None:
        finish = CPP.split(
            "void AWPCombatGameMode::FinishHeadlessSmokeTest()", 1
        )[1]
        self.assertIn("bHeadlessSuppressionStateValid &&", finish)
        self.assertIn("bHeadlessBleedingStateValid &&", finish)


if __name__ == "__main__":
    unittest.main()
