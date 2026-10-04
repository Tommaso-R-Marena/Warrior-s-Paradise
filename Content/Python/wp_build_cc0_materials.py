"""Build Unreal materials from imported CC0/Poly Haven texture groups.

Prerequisite:
  1. Run tools/assets/bootstrap_free_assets.py --polyhaven
  2. Run Content/Python/wp_import_external_assets.py
  3. Run this script in Unreal Editor.

Generated materials are placed under /Game/External/Materials.
"""

from __future__ import annotations

from collections import defaultdict
import re

import unreal


SOURCE_ROOT = "/Game/External/polyhaven"
MATERIAL_ROOT = "/Game/External/Materials"


def log(message: str) -> None:
    unreal.log(f"[WP Material Builder] {message}")


def sanitize(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]+", "_", value).strip("_")


def classify_texture(asset_name: str) -> str | None:
    name = asset_name.lower()

    if any(token in name for token in ("diff", "albedo", "basecolor", "base_color")):
        return "basecolor"
    if "nor_gl" in name or "normal" in name or "_nor" in name:
        return "normal"
    if "rough" in name:
        return "roughness"
    if re.search(r"(^|_)ao($|_)", name) or "ambientocclusion" in name:
        return "ao"

    return None


def configure_texture(texture: unreal.Texture2D, semantic: str) -> None:
    if semantic == "normal":
        texture.set_editor_property(
            "compression_settings",
            unreal.TextureCompressionSettings.TC_NORMALMAP,
        )
        texture.set_editor_property("srgb", False)
    elif semantic in {"roughness", "ao"}:
        texture.set_editor_property("srgb", False)

    texture.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=True)


def group_textures() -> dict[str, dict[str, unreal.Texture2D]]:
    groups: dict[str, dict[str, unreal.Texture2D]] = defaultdict(dict)

    for asset_path in unreal.EditorAssetLibrary.list_assets(
        SOURCE_ROOT,
        recursive=True,
        include_folder=False,
    ):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(asset, unreal.Texture2D):
            continue

        semantic = classify_texture(asset.get_name())
        if not semantic:
            continue

        folder = asset_path.rsplit("/", 1)[0]
        groups[folder][semantic] = asset

    return groups


def create_texture_sample(
    material: unreal.Material,
    texture: unreal.Texture2D,
    x: int,
    y: int,
) -> unreal.MaterialExpressionTextureSample:
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSample,
        x,
        y,
    )
    expression.set_editor_property("texture", texture)
    return expression


def material_name_for(folder: str) -> str:
    relative = folder.removeprefix(SOURCE_ROOT).strip("/")
    parts = [sanitize(part) for part in relative.split("/") if part]
    return "M_WP_" + "_".join(parts[-2:] or ["CC0"])


def build_material(
    folder: str,
    textures: dict[str, unreal.Texture2D],
) -> str | None:
    basecolor = textures.get("basecolor")
    if not basecolor:
        return None

    material_name = material_name_for(folder)
    package_path = MATERIAL_ROOT
    asset_path = f"{package_path}/{material_name}"

    material = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(material, unreal.Material):
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            material_name,
            package_path,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )

    if not isinstance(material, unreal.Material):
        unreal.log_error(f"[WP Material Builder] Could not create {asset_path}")
        return None

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    base_expr = create_texture_sample(material, basecolor, -650, -120)
    unreal.MaterialEditingLibrary.connect_material_property(
        base_expr,
        "RGB",
        unreal.MaterialProperty.MP_BASE_COLOR,
    )

    normal = textures.get("normal")
    if normal:
        configure_texture(normal, "normal")
        normal_expr = create_texture_sample(material, normal, -650, 80)
        unreal.MaterialEditingLibrary.connect_material_property(
            normal_expr,
            "RGB",
            unreal.MaterialProperty.MP_NORMAL,
        )

    roughness = textures.get("roughness")
    if roughness:
        configure_texture(roughness, "roughness")
        rough_expr = create_texture_sample(material, roughness, -650, 280)
        unreal.MaterialEditingLibrary.connect_material_property(
            rough_expr,
            "R",
            unreal.MaterialProperty.MP_ROUGHNESS,
        )

    ao = textures.get("ao")
    if ao:
        configure_texture(ao, "ao")
        ao_expr = create_texture_sample(material, ao, -650, 470)
        unreal.MaterialEditingLibrary.connect_material_property(
            ao_expr,
            "R",
            unreal.MaterialProperty.MP_AMBIENT_OCCLUSION,
        )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return asset_path


def main() -> None:
    groups = group_textures()
    if not groups:
        log(
            "No imported Poly Haven texture groups found under "
            f"{SOURCE_ROOT}. Run the downloader/importer first."
        )
        return

    built: list[str] = []
    for folder, textures in sorted(groups.items()):
        result = build_material(folder, textures)
        if result:
            built.append(result)
            log(f"Built {result}")

    log(f"Finished. Built or refreshed {len(built)} CC0 materials.")


main()
