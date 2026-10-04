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
    "Source/WarriorParadise/Public/Combat/WPWeaponDefinition.h",
    "Source/WarriorParadise/Public/Combat/WPHealthComponent.h",
    "Source/WarriorParadise/Public/Combat/WPBallisticProjectile.h",
    "Source/WarriorParadise/Private/Combat/WPBallisticProjectile.cpp",
    "Source/WarriorParadise/Public/Combat/WPCombatTypes.h",
    "Source/WarriorParadise/Public/Combat/WPFragGrenade.h",
    "Source/WarriorParadise/Private/Combat/WPFragGrenade.cpp",
    "Source/WarriorParadise/Public/Combat/WPSmokeCloud.h",
    "Source/WarriorParadise/Private/Combat/WPSmokeCloud.cpp",
    "Source/WarriorParadise/Public/Combat/WPSmokeGrenade.h",
    "Source/WarriorParadise/Private/Combat/WPSmokeGrenade.cpp",
    "Source/WarriorParadise/Public/Combat/WPSuppressionComponent.h",
    "Source/WarriorParadise/Private/Combat/WPSuppressionComponent.cpp",
    "Source/WarriorParadise/Public/AI/WPCoverPoint.h",
    "Source/WarriorParadise/Private/AI/WPCoverPoint.cpp",
    "Source/WarriorParadise/Public/AI/WPEnemySoldier.h",
    "Source/WarriorParadise/Private/AI/WPEnemySoldier.cpp",
    "Source/WarriorParadise/Public/Game/WPArtilleryStrike.h",
    "Source/WarriorParadise/Private/Game/WPArtilleryStrike.cpp",
    "Source/WarriorParadise/Public/Game/WPBattlefieldDirector.h",
    "Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp",
    "Source/WarriorParadise/Public/Game/WPCaptureObjective.h",
    "Source/WarriorParadise/Private/Game/WPCaptureObjective.cpp",
    "Source/WarriorParadise/Public/Game/WPCombatGameMode.h",
    "Source/WarriorParadise/Public/UI/WPCombatHUD.h",
    "Source/WarriorParadise/Public/World/WPDestructibleCover.h",
    "Source/WarriorParadise/Private/World/WPDestructibleCover.cpp",
    "Source/WarriorParadise/Public/Vehicles/WPArmoredVehicle.h",
    "Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp",
    "Source/WarriorParadise/Public/Vehicles/WPTankShell.h",
    "Source/WarriorParadise/Private/Vehicles/WPTankShell.cpp",
]

FORBIDDEN_MARKERS = (
    "PLACEHOLDER",
    "IMPLEMENT_ME",
    "XXX",
    "TimeSince(",
)


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

    for action in (
        "Aim",
        "ThrowGrenade",
        "ThrowSmoke",
        "Interact",
        "Prone",
        "Melee",
        "Bandage",
        "SwitchSoldier",
        "SquadFollow",
        "SquadAttack",
        "SquadHold",
        "CallArtillery",
    ):
        require_text("Config/DefaultInput.ini", f'ActionName="{action}"')

    require_text("Config/DefaultEngine.ini", "r.SupportLocalFogVolumes=True")

    contracts = (
        ("Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp", "SpawnActor<AWPBallisticProjectile>"),
        ("Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp", "ApplyWeaponDefinition"),
        ("Source/WarriorParadise/Private/Combat/WPBallisticProjectile.cpp", "TryPenetrate"),
        ("Source/WarriorParadise/Private/Combat/WPBallisticProjectile.cpp", "GetHitLocationMultiplier"),
        ("Source/WarriorParadise/Private/Combat/WPBallisticProjectile.cpp", "AddSuppression(NearMissSuppression)"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", 'BindAction(TEXT("ThrowSmoke")'),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", 'BindAction(TEXT("SwitchSoldier")'),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "EnterProne"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "TryReviveSoldier"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "TryMeleeAttack"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "StartBandage"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "SetAccuracyMultiplier"),
        ("Source/WarriorParadise/Private/Combat/WPHealthComponent.cpp", "EnterDownedState"),
        ("Source/WarriorParadise/Private/Combat/WPHealthComponent.cpp", "Heal(float Amount)"),
        ("Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp", "CancelReload"),
        ("Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp", "OnProjectileSpawned.Broadcast"),
        ("Source/WarriorParadise/Private/Combat/WPSmokeCloud.cpp", "SetRadialFogExtinction"),
        ("Source/WarriorParadise/Private/AI/WPEnemySoldier.cpp", "FindBestCover"),
        ("Source/WarriorParadise/Private/AI/WPEnemySoldier.cpp", "GetRecentEnemyContactLocation"),
        ("Source/WarriorParadise/Private/AI/WPEnemySoldier.cpp", "GetRecentEnemyGunshot"),
        ("Source/WarriorParadise/Private/AI/WPEnemySoldier.cpp", "HandleMedicSupport"),
        ("Source/WarriorParadise/Private/AI/WPEnemySoldier.cpp", "HandleEngineerSupport"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "HandleObjectiveCaptured"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "TransferPlayerControl"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "SpawnQueuedFriendlyReplacement"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "IsFriendlySpawnLocationSafe"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "ReportGunshot"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "SwitchPlayerToSquadmate"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "RequestArtillery"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "SpawnArmoredVehicle"),
        ("Source/WarriorParadise/Private/Game/WPCaptureObjective.cpp", "bContested ="),
        ("Source/WarriorParadise/Private/Game/WPCombatGameMode.cpp", "DefaultPawnClass = AWPEnemySoldier::StaticClass()"),
        ("Source/WarriorParadise/Private/World/WPDestructibleCover.cpp", "BreakCover"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "FireCannon"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "FireMachineGun"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "ApplyShellImpact"),
        ("Source/WarriorParadise/Private/Vehicles/WPTankShell.cpp", "DirectArmorDamage"),
    )

    for relative_path, expected in contracts:
        require_text(relative_path, expected)

    checked = 0
    for path in (ROOT / "Source").rglob("*"):
        if not path.is_file() or path.suffix.lower() not in {".h", ".cpp", ".cs"}:
            continue

        text = path.read_text(encoding="utf-8")
        for marker in FORBIDDEN_MARKERS:
            if marker in text:
                fail(f"{marker} marker found in {path.relative_to(ROOT)}")
        checked += 1

    if checked < 43:
        fail(f"unexpectedly small source tree: {checked} source files")

    print(
        "Warrior's Paradise validation passed "
        f"({checked} source files; stance, medicine, perception, squad switching, combat, smoke, support, destruction, and armor contracts checked)."
    )


if __name__ == "__main__":
    main()
