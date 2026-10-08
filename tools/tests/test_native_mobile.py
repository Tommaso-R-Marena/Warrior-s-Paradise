"""Source contracts for the real UE5 mobile infantry/vehicle controls.

These do not compile Unreal C++; they guard control wiring, action releases,
shared HUD hitboxes, and Android packaging expectations until a UE runner exists.
"""
from __future__ import annotations

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Source/WarriorParadise"


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class NativeMobileControlsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.layout = read("Source/WarriorParadise/Public/UI/WPMobileControls.h")
        cls.controller = read("Source/WarriorParadise/Private/Game/WPPlayerController.cpp")
        cls.controller_header = read("Source/WarriorParadise/Public/Game/WPPlayerController.h")
        cls.soldier = read("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp")
        cls.hud = read("Source/WarriorParadise/Private/UI/WPCombatHUD.cpp")

    def test_all_button_actions_use_one_shared_layout(self) -> None:
        specs = re.findall(
            r'\{EWPMobileButton::(\w+),\s*TEXT\("([^"]+)"\),\s*'
            r'FVector2D\(([.0-9]+)f,\s*([.0-9]+)f\),\s*'
            r'FVector2D\(([.0-9]+)f,\s*([.0-9]+)f\)\}',
            self.layout,
        )
        self.assertGreaterEqual(len(specs), 13)
        names = [s[0] for s in specs]
        self.assertEqual(len(names), len(set(names)))
        self.assertIn("Fire", names)
        self.assertIn("Aim", names)
        self.assertIn("Interact", names)
        self.assertIn("Sprint", names)
        for _, _, x, y, half_x, half_y in specs:
            x, y, hx, hy = map(float, (x, y, half_x, half_y))
            self.assertGreater(x-hx, 0)
            self.assertLess(x+hx, 1)
            self.assertGreater(y-hy, 0)
            self.assertLess(y+hy, 1)

    def test_mobile_action_hitboxes_do_not_overlap(self) -> None:
        entries = re.findall(
            r'\{EWPMobileButton::(\w+),\s*TEXT\("[^"]+"\),\s*'
            r'FVector2D\(([.0-9]+)f,\s*([.0-9]+)f\),\s*'
            r'FVector2D\(([.0-9]+)f,\s*([.0-9]+)f\)\}',
            self.layout,
        )
        rectangles = [
            (name, float(x)-float(hx), float(y)-float(hy),
             float(x)+float(hx), float(y)+float(hy))
            for name, x, y, hx, hy in entries
        ]
        self.assertGreaterEqual(len(rectangles), 13)
        for index, a in enumerate(rectangles):
            for b in rectangles[index + 1:]:
                overlap = (a[1] < b[3] and a[3] > b[1] and
                           a[2] < b[4] and a[4] > b[2])
                self.assertFalse(overlap, f"{a[0]} overlaps {b[0]}")

    def test_visible_hud_uses_input_geometry_and_action_availability(self) -> None:
        self.assertIn("WPMobileControls::GetButtons", self.hud)
        self.assertIn("WPMobileControls::GetLabel", self.hud)
        self.assertIn("WPMobileControls::IsAvailable", self.hud)
        self.assertIn("WPMobileControls::HitTest", self.controller)
        self.assertIn("DrawMobileTouchOverlay(Width, Height)", self.hud)

    def test_vehicle_controls_have_individual_modes(self) -> None:
        for name in ("Tank", "LightVehicle", "Emplacement"):
            self.assertIn("EWPMobileMode::" + name, self.layout)
            self.assertIn("EWPMobileMode::" + name, self.controller)
        for thing in ("CANNON", "MG", "SHELL", "EXIT"):
            self.assertIn(thing, self.layout)

    def test_releases_cover_fire_ads_and_sprint(self) -> None:
        for method in ("StopFireInput()", "StopAim()", "StopSprint()"):
            self.assertIn(method, self.soldier)
        for term in ("ReleaseAllMobileActions", "TrackedMobilePawn",
                     "WPMobileControls::IsHeldAction"):
            self.assertIn(term, self.controller)
        for path in ("Vehicles/WPArmoredVehicle", "Vehicles/WPLightVehicle",
                     "World/WPGunEmplacement"):
            c = (SOURCE / "Private" / (path + ".cpp")).read_text(encoding="utf-8")
            self.assertIn("ExecuteMobileAction", c)
            self.assertIn("ApplyMobileLook", c)
            self.assertIn("ApplyMobileMovement", c)

    def test_character_uses_existing_gameplay_rules(self) -> None:
        for term in ("StartFireInput()", "ReloadInput()", "StartBandage()",
                     "ThrowGrenadeInput()", "ThrowSmokeGrenadeInput()",
                     "SwitchSquadmateInput()", "IssueAttackOrder()"):
            self.assertIn(term, self.soldier)
        self.assertIn("ExecuteMobileAction", self.soldier)

    def test_multitouch_look_and_movement_are_distinct(self) -> None:
        for text in ("MobileFingerCount = 10", "EWPMobileGesture::Move",
                     "EWPMobileGesture::Look", "EWPMobileGesture::Button",
                     "GetInputTouchState", "TouchDeadzone", "TouchLookSensitivity"):
            self.assertTrue(text in self.controller or text in self.controller_header, text)

    def test_native_enum_is_not_uht_reflected(self) -> None:
        self.assertNotIn("UENUM", self.layout)
        self.assertNotRegex(
            self.controller_header,
            r"UFUNCTION\([^)]*\)\s*bool IsMobileButtonHeld",
        )
        self.assertLess(
            self.controller_header.index('#include "UI/WPMobileControls.h"'),
            self.controller_header.index('#include "WPPlayerController.generated.h"'),
        )

    def test_android_source_and_packaging_are_present(self) -> None:
        cfg = read("Config/Android/AndroidEngine.ini")
        self.assertNotIn("[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]", read("Config/Android/AndroidGame.ini"))
        script = read("tools/package_android.ps1")
        for term in ("Orientation=Landscape", "bBuildForArm64=True",
                     "MinSDKVersion=26", "TargetSDKVersion=35"):
            self.assertIn(term, cfg)
        for term in ("BuildCookRun", "-cookflavor=$TextureFormat",
                     "-platform=Android", "-package", "-archive",
                     "tools\\check_compile_readiness.py",
                     "-m unittest discover"):
            self.assertIn(term, script)

    def test_mobile_settings_dont_force_desktop_renderer(self) -> None:
        settings = read("Source/WarriorParadise/Private/Game/WPGameUserSettings.cpp")
        self.assertIn("#if PLATFORM_ANDROID || PLATFORM_IOS", settings)
        self.assertIn("Respect Android_Low/Mid/High", settings)
        self.assertIn("Desktop feature CVar defaults must not override Mobile Forward", settings)
        self.assertIn("SetFrameRateLimit(30.0f)", settings)

    def test_stock_touch_interface_disabled_to_avoid_double_joystick(self) -> None:
        cfg = read("Config/DefaultInput.ini")
        self.assertIn("DefaultTouchInterface=None", cfg)
        self.assertIn("bAlwaysShowTouchInterface=False", cfg)


if __name__ == "__main__":
    unittest.main()
