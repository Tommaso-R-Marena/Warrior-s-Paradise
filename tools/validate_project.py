from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED = [
    "WarriorParadise.uproject",
    "Config/DefaultEngine.ini",
    "Config/DefaultGame.ini",
    "Config/DefaultInput.ini",
    "Source/WarriorParadise.Target.cs",
    "Source/WarriorParadiseEditor.Target.cs",
    "Source/WarriorParadise/WarriorParadise.Build.cs",
    "Source/WarriorParadise/WarriorParadise.cpp",
    "Source/WarriorParadise/Public/Characters/WPSoldierCharacter.h",
    "Source/WarriorParadise/Public/Combat/WPWeaponComponent.h",
    "Source/WarriorParadise/Public/Combat/WPHealthComponent.h",
    "Source/WarriorParadise/Public/Combat/WPBallisticProjectile.h",
    "Source/WarriorParadise/Private/Combat/WPBallisticProjectile.cpp",
    "Source/WarriorParadise/Public/Combat/WPCombatTypes.h",
    "Source/WarriorParadise/Public/Combat/WPSuppressionComponent.h",
    "Source/WarriorParadise/Private/Combat/WPSuppressionComponent.cpp",
    "Source/WarriorParadise/Public/AI/WPEnemySoldier.h",
    "Source/WarriorParadise/Public/Game/WPBattlefieldDirector.h",
    "Source/WarriorParadise/Public/Game/WPCaptureObjective.h",
    "Source/WarriorParadise/Private/Game/WPCaptureObjective.cpp",
    "Source/WarriorParadise/Public/Game/WPCombatGameMode.h",
    "Source/WarriorParadise/Public/UI/WPCombatHUD.h",
]

FORBIDDEN_MARKERS = ("PLACEHOLDER", "IMPLEMENT_ME", "XXX")


def fail(message: str) -> None:
    raise SystemExit(f"validation failed: {message}")


def require_text(relative_path: str, expected: str) -> None:
    text = (ROOT / relative_path).read_text(encoding="utf-8")
    if expected not in text:
        fail(f"{relative_path} is missing required contract text: {expected}")


def main() -> None:
    missing = [path for path in REQUIRED if not (ROOT / path).is_file()]
    if missing:
        fail("missing required files: " + ", ".join(missing))

    project = json.loads((ROOT / "WarriorParadise.uproject").read_text(encoding="utf-8"))
    if project.get("EngineAssociation") != "5.8":
        fail("WarriorParadise.uproject must target Unreal Engine 5.8")

    module_names = {entry.get("Name") for entry in project.get("Modules", [])}
    if "WarriorParadise" not in module_names:
        fail("WarriorParadise runtime module is not declared")

    require_text("Config/DefaultInput.ini", 'ActionName="Aim"')
    require_text(
        "Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp",
        "SpawnActor<AWPBallisticProjectile>")
    require_text(
        "Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp",
        'BindAction(TEXT("Aim")')
    require_text(
        "Source/WarriorParadise/Private/Combat/WPBallisticProjectile.cpp",
        "AddSuppression(NearMissSuppression)")
    require_text(
        "Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp",
        "HandleObjectiveCaptured")

    checked = 0
    for path in (ROOT / "Source").rglob("*"):
        if not path.is_file() or path.suffix.lower() not in {".h", ".cpp", ".cs"}:
            continue
        text = path.read_text(encoding="utf-8")
        for marker in FORBIDDEN_MARKERS:
            if marker in text:
                fail(f"{marker} marker found in {path.relative_to(ROOT)}")
        checked += 1

    if checked < 16:
        fail("unexpectedly small source tree")

    print(f"Warrior's Paradise validation passed ({checked} source files checked).")


if __name__ == "__main__":
    main()
