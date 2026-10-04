from __future__ import annotations

import ast
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED = [
    "tools/check_compile_readiness.py",
    "WarriorParadise.uproject",
    "Config/DefaultEngine.ini",
    "Config/DefaultGame.ini",
    "Config/DefaultInput.ini",
    "Config/DefaultScalability.ini",
    "Source/WarriorParadise/Public/Game/WPGameUserSettings.h",
    "Source/WarriorParadise/Private/Game/WPGameUserSettings.cpp",
    "Source/WarriorParadise/Public/Game/WPPlayerController.h",
    "Source/WarriorParadise/Private/Game/WPPlayerController.cpp",
    "docs/SCALABILITY.md",
    "docs/GPU_LESS_MODE.md",
    "Source/WarriorParadise/Public/Game/WPRuntimeMode.h",
    "Source/WarriorParadise/Private/Game/WPRuntimeMode.cpp",
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
    "Source/WarriorParadise/Public/World/WPGunEmplacement.h",
    "Source/WarriorParadise/Private/World/WPGunEmplacement.cpp",
    "Source/WarriorParadise/Public/Vehicles/WPAirBomb.h",
    "Source/WarriorParadise/Private/Vehicles/WPAirBomb.cpp",
    "Source/WarriorParadise/Public/Vehicles/WPAirSupportAircraft.h",
    "Source/WarriorParadise/Private/Vehicles/WPAirSupportAircraft.cpp",
    "Source/WarriorParadise/Public/Vehicles/WPArmoredVehicle.h",
    "Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp",
    "Source/WarriorParadise/Public/Vehicles/WPLightVehicle.h",
    "Source/WarriorParadise/Private/Vehicles/WPLightVehicle.cpp",
    "Source/WarriorParadise/Public/Vehicles/WPTankShell.h",
    "Source/WarriorParadise/Private/Vehicles/WPTankShell.cpp",
    "Source/WarriorParadise/Public/Presentation/WPAirSupportVisualProfile.h",
    "Source/WarriorParadise/Public/Presentation/WPBattlePresentationSet.h",
    "Source/WarriorParadise/Public/Presentation/WPEmplacementVisualProfile.h",
    "Source/WarriorParadise/Public/Presentation/WPExplosionPresentationProfile.h",
    "Source/WarriorParadise/Public/Presentation/WPEnvironmentVisualProfile.h",
    "Source/WarriorParadise/Public/Presentation/WPInfantryLoadoutProfile.h",
    "Source/WarriorParadise/Public/Presentation/WPInfantryVisualProfile.h",
    "Source/WarriorParadise/Public/Presentation/WPLightVehicleVisualProfile.h",
    "Source/WarriorParadise/Public/Presentation/WPSmokePresentationProfile.h",
    "Source/WarriorParadise/Public/Presentation/WPSoldierPresentationComponent.h",
    "Source/WarriorParadise/Private/Presentation/WPSoldierPresentationComponent.cpp",
    "Source/WarriorParadise/Public/Presentation/WPSoldierPresentationProfile.h",
    "Source/WarriorParadise/Public/Presentation/WPVehicleVisualProfile.h",
    "Source/WarriorParadise/Public/Presentation/WPWeaponPresentationComponent.h",
    "Source/WarriorParadise/Private/Presentation/WPWeaponPresentationComponent.cpp",
    "Source/WarriorParadise/Public/Presentation/WPWeaponPresentationProfile.h",
    "tools/assets/free_asset_catalog.json",
    "tools/assets/bootstrap_free_assets.py",
    "tools/assets/check_free_asset_coverage.py",
    "Content/Python/wp_import_external_assets.py",
    "Content/Python/wp_build_cc0_materials.py",
    "Content/Python/wp_setup_free_profiles.py",
    "docs/FREE_ASSET_BOOTSTRAP.md",
    "docs/THIRD_PARTY_ASSETS.md",
    "docs/PRESENTATION_BINDINGS.md",
    "docs/FREE_CONTENT_WORKFLOW.md",
    "docs/LOCAL_BUILD.md",
    "tools/build_local.ps1",
    "tools/build_local.bat",
    "tools/setup_free_content.ps1",
    "tools/setup_free_content.bat",
]

FORBIDDEN_MARKERS = (
    "PLACEHOLDER",
    "IMPLEMENT_ME",
    "XXX",
    "TimeSince(",
    "GetSharedEnemyContact",
)

ALLOWED_LICENSES = {
    "CC0-1.0",
    "MIT",
    "Fab Standard License",
    "Sonniss GDC Bundle License",
}


def fail(message: str) -> None:
    raise SystemExit(f"validation failed: {message}")


def require_text(relative_path: str, expected: str) -> None:
    text = (ROOT / relative_path).read_text(encoding="utf-8")
    if expected not in text:
        fail(f"{relative_path} is missing required contract text: {expected}")


def validate_python() -> int:
    checked = 0
    for path in sorted(ROOT.rglob("*.py")):
        if any(part in {"Binaries", "Intermediate", "Saved"} for part in path.parts):
            continue

        try:
            ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        except SyntaxError as exc:
            fail(
                f"Python syntax error in {path.relative_to(ROOT)}:"
                f"{exc.lineno}:{exc.offset}: {exc.msg}"
            )

        checked += 1

    if checked < 6:
        fail(f"unexpectedly small Python tool surface: {checked}")

    return checked


def validate_asset_catalog() -> tuple[int, int, int]:
    catalog_path = ROOT / "tools/assets/free_asset_catalog.json"
    try:
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(f"invalid free asset catalog JSON: {exc}")

    if catalog.get("schema_version") != 1:
        fail("free asset catalog schema_version must be 1")

    polyhaven = catalog.get("polyhaven")
    if not isinstance(polyhaven, dict):
        fail("free asset catalog is missing polyhaven configuration")

    if polyhaven.get("license") != "CC0-1.0":
        fail("Poly Haven content must remain recorded as CC0-1.0")

    if polyhaven.get("api_base") != "https://api.polyhaven.com":
        fail("unexpected Poly Haven API base")

    if polyhaven.get("api_attribution") != "Powered by Poly Haven":
        fail("Poly Haven API attribution contract was removed")

    buckets = polyhaven.get("buckets")
    if not isinstance(buckets, list) or len(buckets) < 12:
        fail("expected at least 12 curated Poly Haven environment buckets")

    slots: set[str] = set()
    for bucket in buckets:
        if not isinstance(bucket, dict):
            fail("invalid Poly Haven bucket entry")

        slot = bucket.get("slot")
        if not isinstance(slot, str) or not slot:
            fail("Poly Haven bucket is missing slot")

        if slot in slots:
            fail(f"duplicate Poly Haven slot: {slot}")
        slots.add(slot)

        if bucket.get("type") not in {0, 1, 2}:
            fail(f"invalid Poly Haven asset type for slot {slot}")

        queries = bucket.get("queries")
        if not isinstance(queries, list) or not queries:
            fail(f"Poly Haven slot {slot} has no search queries")

    direct = catalog.get("direct_cc0", [])
    if not isinstance(direct, list) or not direct:
        fail("expected at least one direct CC0 source")

    manual = catalog.get("manual_free", [])
    if not isinstance(manual, list) or len(manual) < 8:
        fail("expected a curated manual/provider-free asset set")

    seen_ids: set[str] = set()

    for section_name, entries in (
        ("direct_cc0", direct),
        ("manual_free", manual),
    ):
        for entry in entries:
            if not isinstance(entry, dict):
                fail(f"invalid {section_name} catalog entry")

            asset_id = entry.get("id")
            if not isinstance(asset_id, str) or not asset_id:
                fail(f"{section_name} entry is missing id")

            if asset_id in seen_ids:
                fail(f"duplicate free asset id: {asset_id}")
            seen_ids.add(asset_id)

            for field in ("name", "source", "page_url", "purpose"):
                if not entry.get(field):
                    fail(f"{asset_id} is missing {field}")

            license_name = entry.get("license")
            if license_name and license_name not in ALLOWED_LICENSES:
                fail(
                    f"{asset_id} has unreviewed license label: "
                    f"{license_name}"
                )

            if section_name == "direct_cc0":
                if license_name != "CC0-1.0":
                    fail(f"direct asset {asset_id} must be CC0")
                if not entry.get("download_url"):
                    fail(f"direct asset {asset_id} has no download_url")
                if not entry.get("destination"):
                    fail(f"direct asset {asset_id} has no destination")
            else:
                if not entry.get("acquisition"):
                    fail(f"{asset_id} has no acquisition instructions")
                if not entry.get("distribution"):
                    fail(f"{asset_id} has no distribution policy")

    required_manual = {
        "epic_game_animation_sample",
        "metahuman",
        "quaternius_animation_1",
        "quaternius_animation_2",
        "quaternius_tanks",
        "quaternius_guns",
        "metaworldos_ww2_tanks",
        "fab_rural_cabins",
        "fab_realistic_explosions",
        "fab_muzzle_flash",
        "sonniss_gdc_2026",
        "fab_free_kar98k",
        "fab_free_mp40",
        "fab_free_lee_enfield_mkiii",
        "fab_free_ppsh41_lowpoly",
        "fab_free_military_field_truck",
        "opengameart_ww2_airplane_rigged",
        "opengameart_lowpoly_soviet_jeep",
    }

    missing_manual = required_manual - seen_ids
    if missing_manual:
        fail(
            "free asset catalog lost required candidates: "
            + ", ".join(sorted(missing_manual))
        )

    return len(buckets), len(direct), len(manual)


def validate_project_plugins(project: dict) -> None:
    plugins = {
        entry.get("Name"): bool(entry.get("Enabled"))
        for entry in project.get("Plugins", [])
        if isinstance(entry, dict)
    }

    for required in (
        "PythonScriptPlugin",
        "EditorScriptingUtilities",
        "Niagara",
        "PoseSearch",
    ):
        if not plugins.get(required):
            fail(f"required Unreal plugin is not enabled: {required}")


def main() -> None:
    missing = [path for path in REQUIRED if not (ROOT / path).is_file()]
    if missing:
        fail("missing required files: " + ", ".join(missing))

    project = json.loads(
        (ROOT / "WarriorParadise.uproject").read_text(encoding="utf-8")
    )

    if project.get("EngineAssociation") != "5.8":
        fail("WarriorParadise.uproject must target Unreal Engine 5.8")

    module_names = {
        entry.get("Name")
        for entry in project.get("Modules", [])
        if isinstance(entry, dict)
    }
    if "WarriorParadise" not in module_names:
        fail("WarriorParadise runtime module is not declared")

    validate_project_plugins(project)

    require_text("Source/WarriorParadise/WarriorParadise.Build.cs", '"Niagara"')

    engine_config = (ROOT / "Config/DefaultEngine.ini").read_text(encoding="utf-8")
    if "\\n" in engine_config:
        fail("Config/DefaultEngine.ini contains a literal escaped newline")

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
        "CallAirSupport",
        "CycleGraphicsPreset",
        "AutoGraphicsBenchmark",
    ):
        require_text("Config/DefaultInput.ini", f'ActionName="{action}"')

    require_text("Config/DefaultEngine.ini", "r.SupportLocalFogVolumes=True")

    contracts = (
        ("Source/WarriorParadise/Public/Combat/WPSuppressionComponent.h", "EWPSuppressionState"),
        ("Source/WarriorParadise/Public/Combat/WPSuppressionComponent.h", "PressuredEnterThreshold"),
        ("Source/WarriorParadise/Public/Combat/WPSuppressionComponent.h", "PinnedEnterThreshold"),
        ("Source/WarriorParadise/Private/Combat/WPSuppressionComponent.cpp", "UpdateSuppressionState"),
        ("Source/WarriorParadise/Private/AI/WPEnemySoldier.cpp", "EWPSuppressionState::Pinned"),
        ("Source/WarriorParadise/Private/AI/WPEnemySoldier.cpp", "SuppressionAccuracy"),
        ("Source/WarriorParadise/Private/AI/WPEnemySoldier.cpp", "FireChance = bPinned"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "SuppressionComponent->IsPinned()"),
        ("Source/WarriorParadise/Private/UI/WPCombatHUD.cpp", "PRESSURED"),
        ("Source/WarriorParadise/Private/UI/WPCombatHUD.cpp", "PINNED"),
        ("Source/WarriorParadise/Private/Game/WPCombatGameMode.cpp", "bHeadlessSuppressionStateValid"),
        ("Source/WarriorParadise/Public/Combat/WPWeaponDefinition.h", "bRequiresManualCycle"),
        ("Source/WarriorParadise/Public/Combat/WPWeaponDefinition.h", "ManualCycleDuration"),
        ("Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp", "BeginManualCycle"),
        ("Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp", "FinishManualCycle"),
        ("Source/WarriorParadise/Public/Combat/WPWeaponComponent.h", "OnManualCycleStarted"),
        ("Source/WarriorParadise/Private/Presentation/WPWeaponPresentationComponent.cpp", "HandleManualCycleStarted"),
        ("Source/WarriorParadise/Private/UI/WPCombatHUD.cpp", "CYCLING"),
        ("Content/Python/wp_setup_free_profiles.py", "manual_cycle_seconds=0.68"),
        ("Content/Python/wp_setup_free_profiles.py", "manual_cycle_seconds=0.76"),
        ("Source/WarriorParadise/Private/Game/WPRuntimeMode.cpp", "WPHeadlessSmoke"),
        ("Source/WarriorParadise/Private/Game/WPRuntimeMode.cpp", "nullrhi"),
        ("Source/WarriorParadise/Private/Game/WPCombatGameMode.cpp", "StartHeadlessSmokeTest"),
        ("Source/WarriorParadise/Private/Game/WPCombatGameMode.cpp", "RequestExitWithStatus"),
        ("Source/WarriorParadise/Private/Game/WPGameUserSettings.cpp", "EWPGraphicsPreset::Minimal"),
        ("Source/WarriorParadise/Private/Game/WPGameUserSettings.cpp", "r.DynamicGlobalIlluminationMethod"),
        ("tools/build_local.ps1", "-nullrhi"),
        ("tools/build_local.ps1", "WPIntegratedGraphics"),
        ("Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp", "SpawnActor<AWPBallisticProjectile>"),
        ("Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp", "ApplyWeaponDefinition"),
        ("Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp", "AmmoInMagazine = MagazineSize"),
        ("Source/WarriorParadise/Private/Combat/WPBallisticProjectile.cpp", "TryPenetrate"),
        ("Source/WarriorParadise/Private/Combat/WPBallisticProjectile.cpp", "GetHitLocationMultiplier"),
        ("Source/WarriorParadise/Private/Combat/WPBallisticProjectile.cpp", "AddSuppression(NearMissSuppression)"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", 'BindAction(TEXT("ThrowSmoke")'),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", 'BindAction(TEXT("SwitchSoldier")'),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "EnterProne"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "TryReviveSoldier"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "TryMeleeAttack"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "StartBandage"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "ApplyLoadoutProfile"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "ApplyVisualProfile"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "GetWeaponPresentationTransform"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "GetSmokePresentation"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "AWPGunEmplacement"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "AWPArmoredVehicle"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "AWPLightVehicle"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "RequestAirSupport"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "SoldierPresentationComponent"),
        ("Source/WarriorParadise/Private/Characters/WPSoldierCharacter.cpp", "SetAccuracyMultiplier"),
        ("Source/WarriorParadise/Private/Combat/WPHealthComponent.cpp", "EnterDownedState"),
        ("Source/WarriorParadise/Private/Combat/WPHealthComponent.cpp", "Heal(float Amount)"),
        ("Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp", "CancelReload"),
        ("Source/WarriorParadise/Private/Combat/WPWeaponComponent.cpp", "OnProjectileSpawned.Broadcast"),
        ("Source/WarriorParadise/Private/Combat/WPSmokeCloud.cpp", "SetRadialFogExtinction"),
        ("Source/WarriorParadise/Private/Combat/WPSmokeCloud.cpp", "SpawnSystemAttached"),
        ("Source/WarriorParadise/Private/Combat/WPSmokeGrenade.cpp", "SetPresentationProfile"),
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
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "DA_WP_BattlePresentation_Free"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "ResolveSurfaceMaterial"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "ScatterEnvironmentProps"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "SpawnBuildingVisual"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "SpawnEnterableBuilding"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "GetSmokePresentation"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "SpawnInitialCombinedArms"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "SpawnDefensiveEmplacements"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "SpawnLightVehicle"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "ActiveEnemyLightVehicles"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "NotifyLightVehicleDestroyed"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "SpawnGunEmplacement"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "SpawnAirSupport"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "RequestAirSupport"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "ScheduleAmbientFlyby"),
        ("Source/WarriorParadise/Private/Game/WPBattlefieldDirector.cpp", "WP_Surface_Wood"),
        ("Source/WarriorParadise/Private/Game/WPCaptureObjective.cpp", "bContested ="),
        ("Source/WarriorParadise/Private/Game/WPCombatGameMode.cpp", "DefaultPawnClass = AWPEnemySoldier::StaticClass()"),
        ("Source/WarriorParadise/Private/Game/WPCombatGameMode.cpp", "PlayerControllerClass = AWPPlayerController::StaticClass()"),
        ("Source/WarriorParadise/Private/Game/WPCombatGameMode.cpp", "EnsureInitialHardwareBenchmark"),
        ("Source/WarriorParadise/Private/Game/WPGameUserSettings.cpp", "RunHardwareBenchmark(10, 1.0f, 1.0f)"),
        ("Source/WarriorParadise/Private/Game/WPGameUserSettings.cpp", "SetOverallScalabilityLevel"),
        ("Source/WarriorParadise/Private/Game/WPGameUserSettings.cpp", "67.0f"),
        ("Source/WarriorParadise/Private/Game/WPGameUserSettings.cpp", "78.0f"),
        ("Source/WarriorParadise/Private/Game/WPGameUserSettings.cpp", "90.0f"),
        ("Source/WarriorParadise/Private/Game/WPGameUserSettings.cpp", "100.0f"),
        ("Source/WarriorParadise/Private/Game/WPPlayerController.cpp", "CycleGraphicsPreset"),
        ("Source/WarriorParadise/Private/Game/WPPlayerController.cpp", "RunGraphicsAutoBenchmark"),
        ("Source/WarriorParadise/Private/UI/WPCombatHUD.cpp", "GFX %s   F10 PRESET   F9 AUTO"),
        ("Config/DefaultEngine.ini", "GameUserSettingsClassName=/Script/WarriorParadise.WPGameUserSettings"),
        ("Config/DefaultScalability.ini", 'PerfIndexValues_ResolutionQuality="67 78 90 100 100"'),
        ("Config/DefaultScalability.ini", "[FoliageQuality@0]"),
        ("Config/DefaultScalability.ini", "foliage.DensityScale=0.55"),
        ("Config/DefaultScalability.ini", "[ShadowQuality@3]"),
        ("Source/WarriorParadise/Private/World/WPDestructibleCover.cpp", "BreakCover"),
        ("Source/WarriorParadise/Private/World/WPDestructibleCover.cpp", "ApplyMaterial"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "FireCannon"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "FireMachineGun"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "ApplyShellImpact"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "ApplyVisualProfile"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "CannonMuzzleFlash"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "HandleTankShellImpact"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "DestructionEffect"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "EWPArmorShellType::ArmorPiercing"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "DamageSubsystem"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "AmmoRackCatastrophicDamage"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "TryEnter"),
        ("Source/WarriorParadise/Private/Vehicles/WPArmoredVehicle.cpp", "PlayerFireCannon"),
        ("Source/WarriorParadise/Private/Vehicles/WPLightVehicle.cpp", "EmbarkNearbySquad"),
        ("Source/WarriorParadise/Private/Vehicles/WPLightVehicle.cpp", "FirePlayerMachineGun"),
        ("Source/WarriorParadise/Private/Vehicles/WPLightVehicle.cpp", "FindBestTarget"),
        ("Source/WarriorParadise/Private/Vehicles/WPLightVehicle.cpp", "NotifyLightVehicleDestroyed"),
        ("Source/WarriorParadise/Private/Vehicles/WPAirSupportAircraft.cpp", "InitializeMission"),
        ("Source/WarriorParadise/Private/Vehicles/WPAirSupportAircraft.cpp", "FireStrafeRound"),
        ("Source/WarriorParadise/Private/Vehicles/WPAirSupportAircraft.cpp", "DropBomb"),
        ("Source/WarriorParadise/Private/Vehicles/WPAirBomb.cpp", "Explode"),
        ("Source/WarriorParadise/Private/World/WPGunEmplacement.cpp", "TryUse"),
        ("Source/WarriorParadise/Private/World/WPGunEmplacement.cpp", "AntiTankGun"),
        ("Source/WarriorParadise/Private/World/WPGunEmplacement.cpp", "FireAt"),
        ("Source/WarriorParadise/Private/Vehicles/WPTankShell.cpp", "DirectArmorDamage"),
        ("Source/WarriorParadise/Private/Presentation/WPWeaponPresentationComponent.cpp", "SpawnSystemAtLocation"),
        ("Source/WarriorParadise/Private/Presentation/WPWeaponPresentationComponent.cpp", "PlaySoundAtLocation"),
        ("Source/WarriorParadise/Private/Presentation/WPWeaponPresentationComponent.cpp", "bEnableOcclusion = true"),
        ("Source/WarriorParadise/Private/Presentation/WPWeaponPresentationComponent.cpp", "TracerEffect"),
        ("Source/WarriorParadise/Private/Presentation/WPWeaponPresentationComponent.cpp", "SpawnDecalAtLocation"),
        ("Source/WarriorParadise/Private/Presentation/WPWeaponPresentationComponent.cpp", "DirtImpact"),
        ("Source/WarriorParadise/Private/Presentation/WPSoldierPresentationComponent.cpp", "OnDamageReceived.AddUniqueDynamic"),
        ("Source/WarriorParadise/Private/Presentation/WPSoldierPresentationComponent.cpp", "HandleBandageStarted"),
        ("Source/WarriorParadise/Private/Presentation/WPSoldierPresentationComponent.cpp", "PlayAnimMontage"),
        ("Content/Python/wp_import_external_assets.py", 'DESTINATION_ROOT = "/Game/External"'),
        ("Content/Python/wp_build_cc0_materials.py", 'MATERIAL_ROOT = "/Game/External/Materials"'),
        ("Content/Python/wp_setup_free_profiles.py", 'BATTLE_SET_NAME = "DA_WP_BattlePresentation_Free"'),
        ("Content/Python/wp_setup_free_profiles.py", "WPSmokePresentationProfile"),
        ("Content/Python/wp_setup_free_profiles.py", "WPSoldierPresentationProfile"),
        ("Content/Python/wp_setup_free_profiles.py", "configure_vehicle_presentation"),
        ("Content/Python/wp_setup_free_profiles.py", "WPLightVehicleVisualProfile"),
        ("Content/Python/wp_setup_free_profiles.py", "WPEmplacementVisualProfile"),
        ("Content/Python/wp_setup_free_profiles.py", "WPAirSupportVisualProfile"),
        ("Content/Python/wp_setup_free_profiles.py", "allied_transport"),
        ("Content/Python/wp_setup_free_profiles.py", "allied_aircraft"),
        ("Content/Python/wp_setup_free_profiles.py", "tracer_effect"),
        ("tools/assets/bootstrap_free_assets.py", "safe_extract_zip"),
        ("tools/assets/bootstrap_free_assets.py", "asset-lock.json"),
        ("tools/build_local.ps1", "WarriorParadiseEditor"),
        ("tools/setup_free_content.ps1", "UnrealEditor-Cmd.exe"),
        ("Source/WarriorParadise/Private/UI/WPCombatHUD.cpp", "DrawTankReadout"),
        ("Source/WarriorParadise/Private/UI/WPCombatHUD.cpp", "DrawVehicleReadout"),
        ("Source/WarriorParadise/Private/UI/WPCombatHUD.cpp", "DrawEmplacementReadout"),
        ("Source/WarriorParadise/Private/UI/WPCombatHUD.cpp", "GetAirSupportCooldownRemaining"),
        ("Source/WarriorParadise/Private/UI/WPCombatHUD.cpp", "GetActiveEnemyLightVehicles"),
    )

    for relative_path, expected in contracts:
        require_text(relative_path, expected)

    source_checked = 0
    for path in (ROOT / "Source").rglob("*"):
        if not path.is_file() or path.suffix.lower() not in {".h", ".cpp", ".cs"}:
            continue

        text = path.read_text(encoding="utf-8")
        for marker in FORBIDDEN_MARKERS:
            if marker in text:
                fail(f"{marker} marker found in {path.relative_to(ROOT)}")
        source_checked += 1

    if source_checked < 73:
        fail(f"unexpectedly small source tree: {source_checked} source files")

    python_checked = validate_python()
    buckets, direct_cc0, manual_free = validate_asset_catalog()

    print(
        "Warrior's Paradise validation passed "
        f"({source_checked} source files; {python_checked} Python tools; "
        f"{buckets} Poly Haven slots; {direct_cc0} direct CC0 source(s); "
        f"{manual_free} curated manual/free sources; gameplay, presentation, "
        "asset licensing, import, materials, motion-matching, smoke, soldier "
        "feedback, tracers, decals, surface impacts, enterable buildings, "
        "crewable tanks, subsystem armor damage, transports, armored cars, "
        "emplacements, scripted air support, battlefield ambience, spatial-audio, "
        "destruction, squad, suppression-state behavior, support, first-run auto benchmark and Minimal/Low/Medium/High/Ultra scalability contracts checked)."
    )


if __name__ == "__main__":
    main()
