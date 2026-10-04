from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Source" / "WarriorParadise"

REFLECTION_MACRO = re.compile(r"\bU(?:CLASS|STRUCT|ENUM|INTERFACE)\s*\(")
CLASS_REFLECTION_MACRO = re.compile(r"\bU(?:CLASS|STRUCT|INTERFACE)\s*\(")
INCLUDE_RE = re.compile(r'^\s*#include\s+[<"]([^>"]+)[>"]', re.MULTILINE)
DISALLOWED_REFLECTED_TYPES = re.compile(
    r"\b(?:std::|TUniquePtr\s*<|TSharedPtr\s*<|TWeakPtr\s*<)"
)
ADD_DYNAMIC_RE = re.compile(
    r"\.AddDynamic\s*\(\s*this\s*,\s*&([AU][A-Za-z0-9_]*)::([A-Za-z_][A-Za-z0-9_]*)\s*\)",
    re.DOTALL,
)


class AuditError(RuntimeError):
    pass


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def fail(errors: list[str], path: Path, message: str) -> None:
    errors.append(f"{rel(path)}: {message}")


def reflected_declarations(text: str, macro: str) -> list[str]:
    pattern = re.compile(
        rf"{macro}\s*\([^)]*\)\s*"
        r"(?:class|struct)\s+(?:WARRIORPARADISE_API\s+)?"
        r"([AUF][A-Za-z0-9_]*)\b",
        re.DOTALL,
    )
    return pattern.findall(text)


def has_reflected_handler(header_text: str, method: str) -> bool:
    pattern = re.compile(
        r"UFUNCTION\s*\([^)]*\)\s*"
        r"(?:virtual\s+)?"
        r"(?:[A-Za-z_][A-Za-z0-9_:<>]*[\s*&]+)+"
        + re.escape(method)
        + r"\s*\(",
        re.DOTALL,
    )
    return bool(pattern.search(header_text))


def main() -> int:
    if not SOURCE.is_dir():
        raise AuditError(f"source directory not found: {SOURCE}")

    headers = sorted(SOURCE.rglob("*.h"))
    cpps = sorted(SOURCE.rglob("*.cpp"))
    errors: list[str] = []

    header_by_basename = {path.name: path for path in headers}
    header_text = {path: read(path) for path in headers}

    class_to_header: dict[str, Path] = {}
    for path, text in header_text.items():
        for macro in ("UCLASS", "USTRUCT", "UINTERFACE"):
            for class_name in reflected_declarations(text, macro):
                prior = class_to_header.get(class_name)
                if prior and prior != path:
                    fail(
                        errors,
                        path,
                        f"reflected type {class_name} is also declared in {rel(prior)}",
                    )
                class_to_header[class_name] = path

    for path, text in header_text.items():
        if REFLECTION_MACRO.search(text):
            if not re.search(r"^\s*#pragma\s+once\b", text, re.MULTILINE):
                fail(errors, path, "reflected header is missing #pragma once")

            expected_generated = path.stem + ".generated.h"
            includes = INCLUDE_RE.findall(text)
            if expected_generated not in includes:
                fail(
                    errors,
                    path,
                    f'reflected header is missing #include "{expected_generated}"',
                )
            elif includes[-1] != expected_generated:
                fail(
                    errors,
                    path,
                    f"{expected_generated} must be the final #include",
                )

        if CLASS_REFLECTION_MACRO.search(text) and "GENERATED_BODY()" not in text:
            fail(errors, path, "reflected class/struct header is missing GENERATED_BODY()")

        for match in re.finditer(
            r"UPROPERTY\s*\([^)]*\)\s*([^;]+);",
            text,
            re.DOTALL,
        ):
            declaration = " ".join(match.group(1).split())
            if DISALLOWED_REFLECTED_TYPES.search(declaration):
                fail(
                    errors,
                    path,
                    "unsupported smart/STL type in UPROPERTY: " + declaration[:180],
                )

        for match in re.finditer(
            r"UFUNCTION\s*\([^)]*\)\s*([^;{]+)[;{]",
            text,
            re.DOTALL,
        ):
            signature = " ".join(match.group(1).split())
            if DISALLOWED_REFLECTED_TYPES.search(signature):
                fail(
                    errors,
                    path,
                    "unsupported smart/STL type in UFUNCTION: " + signature[:180],
                )

        for include in INCLUDE_RE.findall(text):
            basename = Path(include).name
            if (
                basename.startswith("WP")
                and basename.endswith(".h")
                and not basename.endswith(".generated.h")
                and basename not in header_by_basename
            ):
                fail(errors, path, f"project include does not exist: {include}")

    for cpp in cpps:
        text = read(cpp)

        for include in INCLUDE_RE.findall(text):
            basename = Path(include).name
            if (
                basename.startswith("WP")
                and basename.endswith(".h")
                and basename not in header_by_basename
            ):
                fail(errors, cpp, f"project include does not exist: {include}")

        if cpp.name.startswith("WP"):
            expected_header = cpp.with_suffix(".h").name
            if expected_header not in header_by_basename:
                fail(
                    errors,
                    cpp,
                    f"implementation has no matching public header {expected_header}",
                )
            elif expected_header not in {Path(item).name for item in INCLUDE_RE.findall(text)}:
                fail(
                    errors,
                    cpp,
                    f"implementation does not include its own header {expected_header}",
                )

        for binding in ADD_DYNAMIC_RE.finditer(text):
            class_name, method = binding.groups()
            header = class_to_header.get(class_name)
            if header is None:
                fail(
                    errors,
                    cpp,
                    f"AddDynamic target class {class_name} has no reflected project header",
                )
                continue

            htext = header_text[header]
            if not re.search(rf"\b{re.escape(method)}\s*\(", htext):
                fail(
                    errors,
                    cpp,
                    f"AddDynamic target {class_name}::{method} is not declared in {rel(header)}",
                )
            elif not has_reflected_handler(htext, method):
                fail(
                    errors,
                    cpp,
                    f"AddDynamic target {class_name}::{method} is not declared with UFUNCTION()",
                )

    build_cs = SOURCE / "WarriorParadise.Build.cs"
    build_text = read(build_cs)
    required_modules = (
        "Core",
        "CoreUObject",
        "Engine",
        "InputCore",
        "AIModule",
        "NavigationSystem",
        "Niagara",
    )
    for module in required_modules:
        if f'"{module}"' not in build_text:
            fail(errors, build_cs, f"required dependency module is missing: {module}")

    game_target = ROOT / "Source" / "WarriorParadise.Target.cs"
    editor_target = ROOT / "Source" / "WarriorParadiseEditor.Target.cs"
    for path, target_type in (
        (game_target, "Game"),
        (editor_target, "Editor"),
    ):
        text = read(path)
        if f"TargetType.{target_type}" not in text:
            fail(errors, path, f"target must declare TargetType.{target_type}")
        if '"WarriorParadise"' not in text:
            fail(errors, path, 'target must include ExtraModuleNames.Add("WarriorParadise")')

    if errors:
        print("Warrior's Paradise compile-readiness audit: FAIL")
        for error in errors:
            print(f"  - {error}")
        print(f"{len(errors)} issue(s) found.")
        return 1

    print(
        "Warrior's Paradise compile-readiness audit: PASS "
        f"({len(headers)} headers, {len(cpps)} C++ translation units)."
    )
    print(
        "Checked UHT generated-header discipline, reflected types, project includes, "
        "dynamic delegate handlers, target files, and module dependencies."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
