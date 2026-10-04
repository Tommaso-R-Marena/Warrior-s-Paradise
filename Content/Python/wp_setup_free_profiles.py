"""Create/update Warrior's Paradise free-resource presentation profiles.

Run after:
  1. importing CC0/free files;
  2. building CC0 materials;
  3. installing any free Fab/Epic/Sonniss content you want to use.

The script is deliberately heuristic. It searches installed assets by names such
as kar98, mp40, enfield, ppsh, sherman, panzer, muzzle, impact and rifle.
Anything not confidently found remains unset and can be assigned in the editor.

The generated battle set is saved at the exact optional path automatically
loaded by AWPBattlefieldDirector:
  /Game/WarriorParadise/Profiles/DA_WP_BattlePresentation_Free
"""

from __future__ import annotations

from collections.abc import Iterable
import re
import unreal


PROFILE_ROOT = "/Game/WarriorParadise/Profiles"
BATTLE_SET_NAME = "DA_WP_BattlePresentation_Free"


def log(message: str) -> None:
    unreal.log(f"[WP Free Profiles] {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"[WP Free Profiles] {message}")


def cls(name: str):
    value = getattr(unreal, name, None)
    if value is None:
        raise RuntimeError(
            f"Required C++ class unreal.{name} is not available. "
            "Compile WarriorParadiseEditor successfully before running this script."
        )
    return value


def create_or_load_data_asset(name: str, class_name: str):
    asset_path = f"{PROFILE_ROOT}/{name}"
    existing = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing:
        return existing

    asset_class = cls(class_name)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", asset_class)

    created = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name,
        PROFILE_ROOT,
        asset_class,
        factory,
    )
    if not created:
        raise RuntimeError(f"Could not create {asset_path}")

    log(f"Created {asset_path}")
    return created


def set_prop(asset, name: str, value) -> bool:
    try:
        asset.set_editor_property(name, value)
        return True
    except Exception as exc:
        warn(f"{asset.get_name()}.{name}: {exc}")
        return False


def list_project_assets() -> list[str]:
    roots = [
        "/Game/External",
        "/Game/Marketplace",
        "/Game/MetaHumans",
        "/Game/GameAnimationSample",
        "/Game",
    ]

    found: set[str] = set()
    for root in roots:
        if not unreal.EditorAssetLibrary.does_directory_exist(root):
            continue
        for path in unreal.EditorAssetLibrary.list_assets(
            root,
            recursive=True,
            include_folder=False,
        ):
            found.add(path)

    return sorted(found)


ASSET_PATHS = list_project_assets()


def score_path(path: str, groups: Iterable[Iterable[str]]) -> int:
    lower = path.lower()
    score = 0

    for group in groups:
        if all(token.lower() in lower for token in group):
            score += 100 + sum(len(token) for token in group)

    return score


def find_asset(class_type, *token_groups: tuple[str, ...]):
    ranked: list[tuple[int, str]] = []

    for path in ASSET_PATHS:
        score = score_path(path, token_groups)
        if score <= 0:
            continue
        ranked.append((score, path))

    ranked.sort(reverse=True)

    for _, path in ranked:
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, class_type):
            log(f"Matched {class_type.__name__}: {path}")
            return asset

    return None


def find_material(slot: str):
    material_class = unreal.MaterialInterface

    search_groups = {
        "ground": [
            ("ground_mud",),
            ("ground_gravel",),
            ("mud",),
            ("dirt",),
        ],
        "plaster": [("wall_plaster",), ("plaster",), ("stucco",)],
        "brick": [("wall_brick",), ("brick",)],
        "stone": [("wall_stone",), ("stone",)],
        "wood": [("wood_weathered",), ("wood",)],
        "metal": [("metal_rusted",), ("rust",)],
        "roof": [("roof_tile",), ("roof", "tile")],
        "concrete": [("concrete_worn",), ("concrete",)],
    }

    return find_asset(material_class, *search_groups[slot])


def configure_weapon(
    asset,
    *,
    weapon_id: str,
    display_name: str,
    damage: float,
    velocity: float,
    range_cm: float,
    hip_spread: float,
    aim_spread: float,
    rpm: float,
    automatic: bool,
    manual_cycle_seconds: float,
    magazine: int,
    reserve: int,
    maximum_reserve: int,
    reload_seconds: float,
    recoil_pitch: float,
    recoil_yaw: float,
) -> None:
    properties = {
        "weapon_id": weapon_id,
        "display_name": display_name,
        "damage": damage,
        "muzzle_velocity_cm_per_second": velocity,
        "range_cm": range_cm,
        "hip_spread_degrees": hip_spread,
        "aim_spread_degrees": aim_spread,
        "rounds_per_minute": rpm,
        "automatic": automatic,
        "requires_manual_cycle": manual_cycle_seconds > 0.0,
        "manual_cycle_duration": max(0.05, manual_cycle_seconds or 0.65),
        "magazine_size": magazine,
        "starting_reserve_ammo": reserve,
        "maximum_reserve_ammo": maximum_reserve,
        "reload_duration": reload_seconds,
        "recoil_pitch_degrees": recoil_pitch,
        "recoil_yaw_degrees": recoil_yaw,
    }

    for key, value in properties.items():
        set_prop(asset, key, value)


def configure_visual_profile(asset, body, weapon) -> None:
    if body:
        set_prop(asset, "third_person_mesh", body)

    if weapon:
        if isinstance(weapon, unreal.SkeletalMesh):
            set_prop(asset, "first_person_weapon_mesh", weapon)
        elif isinstance(weapon, unreal.StaticMesh):
            set_prop(asset, "first_person_static_weapon_mesh", weapon)
            set_prop(asset, "third_person_weapon_mesh", weapon)


def configure_weapon_presentation(asset, *, automatic: bool) -> None:
    niagara_class = unreal.NiagaraSystem
    sound_class = unreal.SoundBase

    muzzle = find_asset(
        niagara_class,
        ("muzzle", "flash"),
        ("muzzle",),
    )
    impact = find_asset(
        niagara_class,
        ("bullet", "impact"),
        ("impact",),
        ("hit",),
    )
    tracer = find_asset(
        niagara_class,
        ("bullet", "tracer"),
        ("tracer",),
        ("projectile", "trail"),
    )

    if muzzle:
        set_prop(asset, "muzzle_flash", muzzle)
    if impact:
        set_prop(asset, "bullet_impact", impact)
    if tracer:
        set_prop(asset, "tracer_effect", tracer)

    material_effects = {
        "dirt_impact": find_asset(
            niagara_class,
            ("dirt", "impact"),
            ("soil", "impact"),
            ("dust", "impact"),
        ),
        "wood_impact": find_asset(
            niagara_class,
            ("wood", "impact"),
            ("wood", "hit"),
            ("splinter",),
        ),
        "stone_impact": find_asset(
            niagara_class,
            ("stone", "impact"),
            ("concrete", "impact"),
            ("dust", "impact"),
        ),
        "metal_impact": find_asset(
            niagara_class,
            ("metal", "impact"),
            ("spark", "impact"),
            ("ricochet",),
        ),
        "flesh_impact": find_asset(
            niagara_class,
            ("flesh", "impact"),
            ("blood", "impact"),
            ("blood",),
        ),
    }

    for property_name, value in material_effects.items():
        if value:
            set_prop(asset, property_name, value)

    if automatic:
        fire = find_asset(
            sound_class,
            ("smg", "fire"),
            ("automatic", "rifle", "fire"),
            ("mp40",),
            ("ppsh",),
        )
    else:
        fire = find_asset(
            sound_class,
            ("rifle", "fire"),
            ("kar98",),
            ("enfield",),
            ("gun", "shot"),
        )

    reload_start = find_asset(
        sound_class,
        ("reload", "start"),
        ("reload",),
    )
    dry = find_asset(
        sound_class,
        ("dry", "fire"),
        ("empty", "click"),
    )
    bullet_impact = find_asset(
        sound_class,
        ("bullet", "impact"),
        ("impact", "metal"),
        ("impact",),
    )
    dirt_impact = find_asset(
        sound_class,
        ("dirt", "impact"),
        ("bullet", "dirt"),
        ("soil", "impact"),
    )
    wood_impact = find_asset(
        sound_class,
        ("wood", "impact"),
        ("bullet", "wood"),
        ("wood", "hit"),
    )
    stone_impact = find_asset(
        sound_class,
        ("stone", "impact"),
        ("concrete", "impact"),
        ("bullet", "concrete"),
    )
    metal_impact = find_asset(
        sound_class,
        ("metal", "impact"),
        ("ricochet",),
        ("bullet", "metal"),
    )
    flesh_impact = find_asset(
        sound_class,
        ("flesh", "impact"),
        ("body", "impact"),
        ("bullet", "flesh"),
    )

    decal_class = unreal.MaterialInterface
    generic_decal = find_asset(
        decal_class,
        ("bullet", "hole"),
        ("bullet", "decal"),
        ("impact", "decal"),
    )
    dirt_decal = find_asset(
        decal_class,
        ("dirt", "bullet", "decal"),
        ("ground", "impact", "decal"),
    )
    wood_decal = find_asset(
        decal_class,
        ("wood", "bullet", "decal"),
        ("wood", "impact", "decal"),
    )
    stone_decal = find_asset(
        decal_class,
        ("concrete", "bullet", "decal"),
        ("stone", "impact", "decal"),
        ("concrete", "decal"),
    )
    metal_decal = find_asset(
        decal_class,
        ("metal", "bullet", "decal"),
        ("metal", "impact", "decal"),
    )

    if generic_decal:
        set_prop(asset, "generic_impact_decal", generic_decal)
    if dirt_decal:
        set_prop(asset, "dirt_impact_decal", dirt_decal)
    if wood_decal:
        set_prop(asset, "wood_impact_decal", wood_decal)
    if stone_decal:
        set_prop(asset, "stone_impact_decal", stone_decal)
    if metal_decal:
        set_prop(asset, "metal_impact_decal", metal_decal)

    if fire:
        set_prop(asset, "fire_sound", fire)
    if reload_start:
        set_prop(asset, "reload_start_sound", reload_start)
    if dry:
        set_prop(asset, "dry_fire_sound", dry)
    if bullet_impact:
        set_prop(asset, "bullet_impact_sound", bullet_impact)
    if dirt_impact:
        set_prop(asset, "dirt_impact_sound", dirt_impact)
    if wood_impact:
        set_prop(asset, "wood_impact_sound", wood_impact)
    if stone_impact:
        set_prop(asset, "stone_impact_sound", stone_impact)
    if metal_impact:
        set_prop(asset, "metal_impact_sound", metal_impact)
    if flesh_impact:
        set_prop(asset, "flesh_impact_sound", flesh_impact)


def configure_explosion_presentation(
    asset,
    *,
    effect_scale: float,
    volume: float,
    sound_groups: list[tuple[str, ...]],
) -> None:
    effect = find_asset(
        unreal.NiagaraSystem,
        ("explosion",),
        ("blast",),
        ("fire", "explosion"),
    )
    sound = find_asset(
        unreal.SoundBase,
        *sound_groups,
        ("explosion",),
        ("blast",),
    )

    if effect:
        set_prop(asset, "explosion_effect", effect)
    if sound:
        set_prop(asset, "explosion_sound", sound)

    set_prop(
        asset,
        "effect_scale",
        unreal.Vector(effect_scale, effect_scale, effect_scale),
    )
    set_prop(asset, "volume", volume)


def configure_vehicle_presentation(asset) -> None:
    cannon_muzzle = find_asset(
        unreal.NiagaraSystem,
        ("tank", "muzzle"),
        ("cannon", "muzzle"),
        ("muzzle", "flash"),
    )
    machine_muzzle = find_asset(
        unreal.NiagaraSystem,
        ("machine", "gun", "muzzle"),
        ("muzzle", "flash"),
    )
    shell_impact = find_asset(
        unreal.NiagaraSystem,
        ("tank", "impact"),
        ("shell", "impact"),
        ("explosion",),
    )
    destruction = find_asset(
        unreal.NiagaraSystem,
        ("vehicle", "explosion"),
        ("tank", "explosion"),
        ("explosion",),
    )

    cannon_sound = find_asset(
        unreal.SoundBase,
        ("tank", "cannon"),
        ("cannon", "fire"),
        ("tank", "fire"),
    )
    machine_sound = find_asset(
        unreal.SoundBase,
        ("machine", "gun", "fire"),
        ("mg", "fire"),
        ("automatic", "fire"),
    )
    impact_sound = find_asset(
        unreal.SoundBase,
        ("shell", "impact"),
        ("tank", "impact"),
        ("heavy", "impact"),
        ("explosion",),
    )
    destruction_sound = find_asset(
        unreal.SoundBase,
        ("vehicle", "explosion"),
        ("tank", "explosion"),
        ("explosion",),
    )

    for property_name, value in {
        "cannon_muzzle_flash": cannon_muzzle,
        "machine_gun_muzzle_flash": machine_muzzle,
        "shell_impact_effect": shell_impact,
        "destruction_effect": destruction,
        "cannon_sound": cannon_sound,
        "machine_gun_sound": machine_sound,
        "shell_impact_sound": impact_sound,
        "destruction_sound": destruction_sound,
    }.items():
        if value:
            set_prop(asset, property_name, value)


def configure_light_vehicle_presentation(asset, *, armored: bool) -> None:
    muzzle = find_asset(
        unreal.NiagaraSystem,
        ("machine", "gun", "muzzle"),
        ("muzzle", "flash"),
    )
    destruction = find_asset(
        unreal.NiagaraSystem,
        ("vehicle", "explosion"),
        ("vehicle", "fire"),
        ("explosion",),
    )
    engine = find_asset(
        unreal.SoundBase,
        ("truck", "engine"),
        ("vehicle", "engine"),
        ("engine", "loop"),
    )
    machine_gun = find_asset(
        unreal.SoundBase,
        ("machine", "gun", "fire"),
        ("mg", "fire"),
        ("automatic", "fire"),
    )
    destruction_sound = find_asset(
        unreal.SoundBase,
        ("vehicle", "explosion"),
        ("vehicle", "destroy"),
        ("explosion",),
    )

    if muzzle and armored:
        set_prop(asset, "machine_gun_muzzle_flash", muzzle)
    if destruction:
        set_prop(asset, "destruction_effect", destruction)
    if engine:
        set_prop(asset, "engine_loop", engine)
    if machine_gun and armored:
        set_prop(asset, "machine_gun_sound", machine_gun)
    if destruction_sound:
        set_prop(asset, "destruction_sound", destruction_sound)


def configure_emplacement_presentation(asset, *, anti_tank: bool) -> None:
    muzzle = find_asset(
        unreal.NiagaraSystem,
        ("cannon", "muzzle") if anti_tank else ("machine", "gun", "muzzle"),
        ("muzzle", "flash"),
    )
    destruction = find_asset(
        unreal.NiagaraSystem,
        ("emplacement", "explosion"),
        ("vehicle", "explosion"),
        ("explosion",),
    )

    if anti_tank:
        fire_sound = find_asset(
            unreal.SoundBase,
            ("anti", "tank", "gun"),
            ("cannon", "fire"),
            ("artillery", "fire"),
        )
    else:
        fire_sound = find_asset(
            unreal.SoundBase,
            ("heavy", "machine", "gun"),
            ("machine", "gun", "fire"),
            ("mg42",),
        )

    destruction_sound = find_asset(
        unreal.SoundBase,
        ("emplacement", "destroy"),
        ("vehicle", "explosion"),
        ("explosion",),
    )

    if muzzle:
        set_prop(asset, "muzzle_flash", muzzle)
    if destruction:
        set_prop(asset, "destruction_effect", destruction)
    if fire_sound:
        set_prop(asset, "fire_sound", fire_sound)
    if destruction_sound:
        set_prop(asset, "destruction_sound", destruction_sound)


def configure_aircraft_presentation(asset) -> None:
    muzzle = find_asset(
        unreal.NiagaraSystem,
        ("aircraft", "muzzle"),
        ("machine", "gun", "muzzle"),
        ("muzzle", "flash"),
    )
    engine = find_asset(
        unreal.SoundBase,
        ("aircraft", "engine"),
        ("plane", "engine"),
        ("propeller",),
    )
    flyby = find_asset(
        unreal.SoundBase,
        ("aircraft", "flyby"),
        ("plane", "flyby"),
        ("flyby",),
    )
    gun = find_asset(
        unreal.SoundBase,
        ("aircraft", "gun"),
        ("fighter", "gun"),
        ("machine", "gun", "fire"),
    )
    bomb_release = find_asset(
        unreal.SoundBase,
        ("bomb", "release"),
        ("bomb", "drop"),
        ("ordnance", "release"),
    )

    if muzzle:
        set_prop(asset, "gun_muzzle_flash", muzzle)
    if engine:
        set_prop(asset, "engine_loop", engine)
    if flyby:
        set_prop(asset, "flyby_sound", flyby)
    if gun:
        set_prop(asset, "gun_sound", gun)
    if bomb_release:
        set_prop(asset, "bomb_release_sound", bomb_release)


def configure_soldier_presentation(asset) -> None:
    hit_effect = find_asset(
        unreal.NiagaraSystem,
        ("blood", "impact"),
        ("flesh", "impact"),
        ("hit", "blood"),
    )
    death_effect = find_asset(
        unreal.NiagaraSystem,
        ("blood", "death"),
        ("death", "blood"),
        ("blood",),
    )

    sounds = {
        "pain_sound": find_asset(
            unreal.SoundBase,
            ("soldier", "pain"),
            ("pain", "voice"),
            ("pain",),
        ),
        "downed_sound": find_asset(
            unreal.SoundBase,
            ("downed",),
            ("wounded", "voice"),
        ),
        "revive_sound": find_asset(
            unreal.SoundBase,
            ("revive",),
            ("medical",),
        ),
        "death_sound": find_asset(
            unreal.SoundBase,
            ("soldier", "death"),
            ("death", "voice"),
        ),
        "melee_sound": find_asset(
            unreal.SoundBase,
            ("melee", "hit"),
            ("punch",),
            ("body", "impact"),
        ),
        "bandage_start_sound": find_asset(
            unreal.SoundBase,
            ("bandage",),
            ("cloth", "medical"),
            ("cloth",),
        ),
        "bandage_complete_sound": find_asset(
            unreal.SoundBase,
            ("bandage", "complete"),
            ("medical", "complete"),
        ),
        "footstep_dirt": find_asset(
            unreal.SoundBase,
            ("footstep", "dirt"),
            ("step", "dirt"),
            ("foot", "soil"),
        ),
        "footstep_wood": find_asset(
            unreal.SoundBase,
            ("footstep", "wood"),
            ("step", "wood"),
        ),
        "footstep_stone": find_asset(
            unreal.SoundBase,
            ("footstep", "stone"),
            ("footstep", "concrete"),
            ("step", "stone"),
        ),
        "footstep_metal": find_asset(
            unreal.SoundBase,
            ("footstep", "metal"),
            ("step", "metal"),
        ),
        "crawl_sound": find_asset(
            unreal.SoundBase,
            ("crawl",),
            ("cloth", "movement"),
            ("gear", "movement"),
        ),
    }

    montages = {
        "hit_react_montage": find_asset(
            unreal.AnimMontage,
            ("hit", "react"),
            ("hitreact",),
        ),
        "downed_montage": find_asset(
            unreal.AnimMontage,
            ("downed",),
            ("wounded",),
        ),
        "revive_montage": find_asset(
            unreal.AnimMontage,
            ("revive",),
            ("get", "up"),
        ),
        "death_montage": find_asset(
            unreal.AnimMontage,
            ("death",),
            ("die",),
        ),
        "melee_montage": find_asset(
            unreal.AnimMontage,
            ("melee",),
            ("punch",),
        ),
        "bandage_montage": find_asset(
            unreal.AnimMontage,
            ("bandage",),
            ("medical",),
        ),
    }

    if hit_effect:
        set_prop(asset, "hit_effect", hit_effect)
    if death_effect:
        set_prop(asset, "death_effect", death_effect)

    for property_name, value in sounds.items():
        if value:
            set_prop(asset, property_name, value)

    for property_name, value in montages.items():
        if value:
            set_prop(asset, property_name, value)


def configure_loadout(
    asset,
    *,
    display_name: str,
    weapon,
    visuals,
    presentation,
    soldier_presentation,
    frag: int,
    smoke: int,
    medkits: int,
) -> None:
    set_prop(asset, "display_name", display_name)
    set_prop(asset, "weapon_definition", weapon)
    set_prop(asset, "visual_profile", visuals)
    set_prop(asset, "weapon_presentation_profile", presentation)
    set_prop(asset, "soldier_presentation_profile", soldier_presentation)
    set_prop(asset, "frag_grenades", frag)
    set_prop(asset, "smoke_grenades", smoke)
    set_prop(asset, "medkits", medkits)


def save(asset) -> None:
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)


def main() -> None:
    body = find_asset(
        unreal.SkeletalMesh,
        ("metahuman", "body"),
        ("ww2", "soldier"),
        ("soldier",),
    )

    locomotion_anim_blueprint = find_asset(
        unreal.AnimBlueprint,
        ("abp", "sandboxcharacter"),
        ("game", "animation", "sample"),
        ("motion", "matching"),
    )

    weapon_meshes = {
        "lee": find_asset(
            unreal.StaticMesh,
            ("lee", "enfield"),
            ("enfield",),
        ),
        "ppsh": find_asset(
            unreal.StaticMesh,
            ("ppsh",),
            ("ppsh41",),
        ),
        "kar98": find_asset(
            unreal.StaticMesh,
            ("kar98",),
            ("karabiner",),
        ),
        "mp40": find_asset(
            unreal.StaticMesh,
            ("mp40",),
            ("mp_40",),
            ("maschinenpistole",),
        ),
    }

    vehicle_meshes = {
        "allies": find_asset(
            unreal.StaticMesh,
            ("sherman",),
            ("m4", "tank"),
        ),
        "axis": find_asset(
            unreal.StaticMesh,
            ("panzer", "iv"),
            ("panzer",),
            ("panther",),
        ),
    }

    light_vehicle_meshes = {
        "allied_transport": find_asset(
            unreal.StaticMesh,
            ("military", "truck"),
            ("field", "truck"),
            ("transport", "truck"),
            ("truck",),
            ("jeep",),
        ),
        "allied_armored": find_asset(
            unreal.StaticMesh,
            ("armored", "car"),
            ("armoured", "car"),
            ("halftrack",),
            ("jeep", "mounted"),
        ),
        "axis_armored": find_asset(
            unreal.StaticMesh,
            ("sdkfz",),
            ("armored", "car"),
            ("armoured", "car"),
            ("halftrack",),
        ),
    }

    emplacement_meshes = {
        "hmg": find_asset(
            unreal.StaticMesh,
            ("mg42",),
            ("heavy", "machine", "gun"),
            ("machine", "gun"),
            ("hmg",),
        ),
        "at": find_asset(
            unreal.StaticMesh,
            ("pak", "40"),
            ("anti", "tank", "gun"),
            ("at", "gun"),
            ("field", "gun"),
        ),
    }

    aircraft_meshes = {
        "allies": find_asset(
            unreal.StaticMesh,
            ("spitfire",),
            ("p51",),
            ("mustang",),
            ("allied", "fighter"),
            ("ww2", "airplane"),
        ),
        "axis": find_asset(
            unreal.StaticMesh,
            ("bf109",),
            ("bf", "109"),
            ("messerschmitt",),
            ("axis", "fighter"),
            ("ww2", "airplane"),
        ),
    }

    weapons = {
        "lee": create_or_load_data_asset("DA_WP_Weapon_LeeEnfield", "WPWeaponDefinition"),
        "ppsh": create_or_load_data_asset("DA_WP_Weapon_PPSh41", "WPWeaponDefinition"),
        "kar98": create_or_load_data_asset("DA_WP_Weapon_Kar98k", "WPWeaponDefinition"),
        "mp40": create_or_load_data_asset("DA_WP_Weapon_MP40", "WPWeaponDefinition"),
    }

    configure_weapon(
        weapons["lee"],
        weapon_id="lee_enfield_mkiii",
        display_name="Lee-Enfield Mk III",
        damage=88.0,
        velocity=74400.0,
        range_cm=90000.0,
        hip_spread=1.35,
        aim_spread=0.10,
        rpm=55.0,
        automatic=False,
        manual_cycle_seconds=0.68,
        magazine=10,
        reserve=70,
        maximum_reserve=120,
        reload_seconds=3.2,
        recoil_pitch=1.75,
        recoil_yaw=0.28,
    )

    configure_weapon(
        weapons["kar98"],
        weapon_id="kar98k",
        display_name="Karabiner 98k",
        damage=92.0,
        velocity=76000.0,
        range_cm=95000.0,
        hip_spread=1.40,
        aim_spread=0.09,
        rpm=52.0,
        automatic=False,
        manual_cycle_seconds=0.76,
        magazine=5,
        reserve=60,
        maximum_reserve=100,
        reload_seconds=3.0,
        recoil_pitch=1.95,
        recoil_yaw=0.30,
    )

    configure_weapon(
        weapons["mp40"],
        weapon_id="mp40",
        display_name="MP 40",
        damage=31.0,
        velocity=40000.0,
        range_cm=40000.0,
        hip_spread=1.55,
        aim_spread=0.28,
        rpm=500.0,
        automatic=True,
        manual_cycle_seconds=0.0,
        magazine=32,
        reserve=160,
        maximum_reserve=224,
        reload_seconds=2.5,
        recoil_pitch=0.82,
        recoil_yaw=0.32,
    )

    configure_weapon(
        weapons["ppsh"],
        weapon_id="ppsh41",
        display_name="PPSh-41",
        damage=27.0,
        velocity=48800.0,
        range_cm=42000.0,
        hip_spread=1.60,
        aim_spread=0.30,
        rpm=900.0,
        automatic=True,
        manual_cycle_seconds=0.0,
        magazine=71,
        reserve=213,
        maximum_reserve=284,
        reload_seconds=2.9,
        recoil_pitch=0.68,
        recoil_yaw=0.38,
    )

    visuals = {}
    for key in ("lee", "ppsh", "kar98", "mp40"):
        visual = create_or_load_data_asset(
            f"DA_WP_Visual_{key.upper()}",
            "WPInfantryVisualProfile",
        )
        configure_visual_profile(visual, body, weapon_meshes[key])
        visuals[key] = visual

    if body and locomotion_anim_blueprint:
        try:
            body_skeleton = body.get_editor_property("skeleton")
            target_skeleton = locomotion_anim_blueprint.get_editor_property(
                "target_skeleton"
            )

            if body_skeleton == target_skeleton:
                anim_class = locomotion_anim_blueprint.generated_class()
                for visual in visuals.values():
                    set_prop(visual, "third_person_anim_class", anim_class)
                log(
                    "Assigned compatible Game Animation Sample/Motion Matching "
                    "AnimBlueprint to free infantry visual profiles."
                )
            else:
                warn(
                    "A Motion Matching AnimBlueprint was found, but its target "
                    "skeleton does not match the selected body. Retarget it in "
                    "UE5, then rerun this script or assign the retargeted AnimBP "
                    "to the visual profiles."
                )
        except Exception as exc:
            warn(f"Could not verify animation skeleton compatibility: {exc}")

    rifle_presentation = create_or_load_data_asset(
        "DA_WP_Presentation_Rifle",
        "WPWeaponPresentationProfile",
    )
    smg_presentation = create_or_load_data_asset(
        "DA_WP_Presentation_SMG",
        "WPWeaponPresentationProfile",
    )
    configure_weapon_presentation(rifle_presentation, automatic=False)
    configure_weapon_presentation(smg_presentation, automatic=True)

    soldier_presentation = create_or_load_data_asset(
        "DA_WP_Presentation_Soldier",
        "WPSoldierPresentationProfile",
    )
    configure_soldier_presentation(soldier_presentation)

    loadout_specs = {
        "AlliedRifleman": ("lee", rifle_presentation, 2, 1, 1),
        "AlliedAssault": ("ppsh", smg_presentation, 1, 1, 1),
        "AlliedMedic": ("lee", rifle_presentation, 1, 2, 2),
        "AlliedEngineer": ("lee", rifle_presentation, 2, 1, 1),
        "AxisRifleman": ("kar98", rifle_presentation, 2, 1, 1),
        "AxisAssault": ("mp40", smg_presentation, 1, 1, 1),
        "AxisMedic": ("kar98", rifle_presentation, 1, 2, 2),
        "AxisEngineer": ("kar98", rifle_presentation, 2, 1, 1),
    }

    loadouts = {}
    for name, (weapon_key, presentation, frag, smoke, medkits) in loadout_specs.items():
        asset = create_or_load_data_asset(
            f"DA_WP_Loadout_{name}",
            "WPInfantryLoadoutProfile",
        )
        configure_loadout(
            asset,
            display_name=name,
            weapon=weapons[weapon_key],
            visuals=visuals[weapon_key],
            presentation=presentation,
            soldier_presentation=soldier_presentation,
            frag=frag,
            smoke=smoke,
            medkits=medkits,
        )
        loadouts[name] = asset

    allied_vehicle = create_or_load_data_asset(
        "DA_WP_Vehicle_Allied_Free",
        "WPVehicleVisualProfile",
    )
    axis_vehicle = create_or_load_data_asset(
        "DA_WP_Vehicle_Axis_Free",
        "WPVehicleVisualProfile",
    )

    if vehicle_meshes["allies"]:
        set_prop(allied_vehicle, "hull_mesh", vehicle_meshes["allies"])
    if vehicle_meshes["axis"]:
        set_prop(axis_vehicle, "hull_mesh", vehicle_meshes["axis"])

    configure_vehicle_presentation(allied_vehicle)
    configure_vehicle_presentation(axis_vehicle)

    allied_transport = create_or_load_data_asset(
        "DA_WP_LightVehicle_AlliedTransport",
        "WPLightVehicleVisualProfile",
    )
    allied_armored_car = create_or_load_data_asset(
        "DA_WP_LightVehicle_AlliedArmoredCar",
        "WPLightVehicleVisualProfile",
    )
    axis_armored_car = create_or_load_data_asset(
        "DA_WP_LightVehicle_AxisArmoredCar",
        "WPLightVehicleVisualProfile",
    )

    if light_vehicle_meshes["allied_transport"]:
        set_prop(
            allied_transport,
            "body_mesh",
            light_vehicle_meshes["allied_transport"],
        )
    if light_vehicle_meshes["allied_armored"]:
        set_prop(
            allied_armored_car,
            "body_mesh",
            light_vehicle_meshes["allied_armored"],
        )
    if light_vehicle_meshes["axis_armored"]:
        set_prop(
            axis_armored_car,
            "body_mesh",
            light_vehicle_meshes["axis_armored"],
        )

    configure_light_vehicle_presentation(
        allied_transport,
        armored=False,
    )
    configure_light_vehicle_presentation(
        allied_armored_car,
        armored=True,
    )
    configure_light_vehicle_presentation(
        axis_armored_car,
        armored=True,
    )

    allied_hmg = create_or_load_data_asset(
        "DA_WP_Emplacement_AlliedHMG",
        "WPEmplacementVisualProfile",
    )
    allied_at = create_or_load_data_asset(
        "DA_WP_Emplacement_AlliedAT",
        "WPEmplacementVisualProfile",
    )
    axis_hmg = create_or_load_data_asset(
        "DA_WP_Emplacement_AxisHMG",
        "WPEmplacementVisualProfile",
    )
    axis_at = create_or_load_data_asset(
        "DA_WP_Emplacement_AxisAT",
        "WPEmplacementVisualProfile",
    )

    for hmg_profile in (allied_hmg, axis_hmg):
        if emplacement_meshes["hmg"]:
            set_prop(
                hmg_profile,
                "gun_mesh",
                emplacement_meshes["hmg"],
            )
        configure_emplacement_presentation(
            hmg_profile,
            anti_tank=False,
        )

    for at_profile in (allied_at, axis_at):
        if emplacement_meshes["at"]:
            set_prop(
                at_profile,
                "gun_mesh",
                emplacement_meshes["at"],
            )
        configure_emplacement_presentation(
            at_profile,
            anti_tank=True,
        )

    allied_aircraft = create_or_load_data_asset(
        "DA_WP_Aircraft_Allied",
        "WPAirSupportVisualProfile",
    )
    axis_aircraft = create_or_load_data_asset(
        "DA_WP_Aircraft_Axis",
        "WPAirSupportVisualProfile",
    )

    if aircraft_meshes["allies"]:
        set_prop(
            allied_aircraft,
            "aircraft_mesh",
            aircraft_meshes["allies"],
        )
    if aircraft_meshes["axis"]:
        set_prop(
            axis_aircraft,
            "aircraft_mesh",
            aircraft_meshes["axis"],
        )

    configure_aircraft_presentation(allied_aircraft)
    configure_aircraft_presentation(axis_aircraft)

    frag_explosion = create_or_load_data_asset(
        "DA_WP_Explosion_Frag",
        "WPExplosionPresentationProfile",
    )
    artillery_explosion = create_or_load_data_asset(
        "DA_WP_Explosion_Artillery",
        "WPExplosionPresentationProfile",
    )

    configure_explosion_presentation(
        frag_explosion,
        effect_scale=0.85,
        volume=0.95,
        sound_groups=[
            ("grenade", "explosion"),
            ("frag", "explosion"),
            ("small", "explosion"),
        ],
    )
    configure_explosion_presentation(
        artillery_explosion,
        effect_scale=1.85,
        volume=1.0,
        sound_groups=[
            ("artillery", "explosion"),
            ("heavy", "explosion"),
            ("large", "explosion"),
        ],
    )

    smoke_presentation = create_or_load_data_asset(
        "DA_WP_Presentation_Smoke",
        "WPSmokePresentationProfile",
    )

    smoke_effect = find_asset(
        unreal.NiagaraSystem,
        ("smoke", "grenade"),
        ("smoke", "cloud"),
        ("smoke",),
    )
    smoke_deploy = find_asset(
        unreal.SoundBase,
        ("smoke", "grenade", "deploy"),
        ("smoke", "deploy"),
        ("grenade", "hiss"),
    )
    smoke_hiss = find_asset(
        unreal.SoundBase,
        ("smoke", "hiss"),
        ("hiss", "loop"),
        ("gas", "hiss"),
    )

    if smoke_effect:
        set_prop(smoke_presentation, "smoke_effect", smoke_effect)
    if smoke_deploy:
        set_prop(smoke_presentation, "deploy_sound", smoke_deploy)
    if smoke_hiss:
        set_prop(smoke_presentation, "hiss_loop", smoke_hiss)

    environment = create_or_load_data_asset(
        "DA_WP_Environment_Free",
        "WPEnvironmentVisualProfile",
    )

    village_building = find_asset(
        unreal.StaticMesh,
        ("rural", "cabin"),
        ("cabin",),
        ("village", "house"),
        ("house",),
        ("building",),
    )

    if village_building:
        set_prop(environment, "village_building_mesh", village_building)

    tree_mesh = find_asset(
        unreal.StaticMesh,
        ("foliage_tree",),
        ("tree",),
    )
    crate_mesh = find_asset(
        unreal.StaticMesh,
        ("prop_crate",),
        ("crate",),
    )
    barrel_mesh = find_asset(
        unreal.StaticMesh,
        ("prop_barrel",),
        ("barrel",),
    )
    sky_cubemap = find_asset(
        unreal.TextureCube,
        ("hdri_overcast",),
        ("overcast",),
        ("cloudy",),
    )

    if tree_mesh:
        set_prop(environment, "tree_mesh", tree_mesh)
    if crate_mesh:
        set_prop(environment, "crate_mesh", crate_mesh)
    if barrel_mesh:
        set_prop(environment, "barrel_mesh", barrel_mesh)
    if sky_cubemap:
        set_prop(environment, "sky_cubemap", sky_cubemap)

    cloud_material = find_asset(
        unreal.MaterialInterface,
        ("volumetric", "cloud"),
        ("cloud", "volume"),
    )
    ambient_loop = find_asset(
        unreal.SoundBase,
        ("battlefield", "ambience"),
        ("war", "ambience"),
        ("outdoor", "ambience"),
        ("village", "ambience"),
    )
    wind_loop = find_asset(
        unreal.SoundBase,
        ("wind", "ambience"),
        ("wind",),
    )
    distant_battle = find_asset(
        unreal.SoundBase,
        ("distant", "battle"),
        ("distant", "gunfire"),
        ("distant", "artillery"),
        ("battle", "distant"),
    )

    if cloud_material:
        set_prop(environment, "cloud_material", cloud_material)
    if ambient_loop:
        set_prop(environment, "ambient_loop", ambient_loop)
    if wind_loop:
        set_prop(environment, "wind_loop", wind_loop)
    if distant_battle:
        set_prop(environment, "distant_battle_one_shot", distant_battle)

    for slot, property_name in {
        "ground": "ground_material",
        "plaster": "plaster_material",
        "brick": "brick_material",
        "stone": "stone_material",
        "wood": "wood_material",
        "metal": "metal_material",
        "roof": "roof_material",
        "concrete": "concrete_material",
    }.items():
        material = find_material(slot)
        if material:
            set_prop(environment, property_name, material)

    battle = create_or_load_data_asset(
        BATTLE_SET_NAME,
        "WPBattlePresentationSet",
    )

    mapping = {
        "allied_rifleman": loadouts["AlliedRifleman"],
        "allied_assault": loadouts["AlliedAssault"],
        "allied_medic": loadouts["AlliedMedic"],
        "allied_engineer": loadouts["AlliedEngineer"],
        "axis_rifleman": loadouts["AxisRifleman"],
        "axis_assault": loadouts["AxisAssault"],
        "axis_medic": loadouts["AxisMedic"],
        "axis_engineer": loadouts["AxisEngineer"],
        "allied_vehicle": allied_vehicle,
        "axis_vehicle": axis_vehicle,
        "allied_transport": allied_transport,
        "allied_armored_car": allied_armored_car,
        "axis_armored_car": axis_armored_car,
        "allied_hmg": allied_hmg,
        "allied_at_gun": allied_at,
        "axis_hmg": axis_hmg,
        "axis_at_gun": axis_at,
        "allied_aircraft": allied_aircraft,
        "axis_aircraft": axis_aircraft,
        "environment": environment,
        "frag_explosion": frag_explosion,
        "artillery_explosion": artillery_explosion,
        "smoke_presentation": smoke_presentation,
    }

    for property_name, value in mapping.items():
        set_prop(battle, property_name, value)

    all_assets = [
        *weapons.values(),
        *visuals.values(),
        rifle_presentation,
        smg_presentation,
        soldier_presentation,
        *loadouts.values(),
        allied_vehicle,
        axis_vehicle,
        allied_transport,
        allied_armored_car,
        axis_armored_car,
        allied_hmg,
        allied_at,
        axis_hmg,
        axis_at,
        allied_aircraft,
        axis_aircraft,
        frag_explosion,
        artillery_explosion,
        smoke_presentation,
        environment,
        battle,
    ]

    for asset in all_assets:
        save(asset)

    log(
        "Free presentation preset is ready. Restart PIE so "
        "AWPBattlefieldDirector reloads DA_WP_BattlePresentation_Free."
    )


main()
