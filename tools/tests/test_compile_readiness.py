"""Dependency-free regression tests for our static Unreal source audit.

These do not replace UnrealHeaderTool or a real UE 5.8 compile.
"""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import check_compile_readiness as audit


class ReflectionAuditTests(unittest.TestCase):
    def test_reflected_project_type_with_export_macro(self) -> None:
        header = (
            "UCLASS(BlueprintType)\n"
            "class WARRIORPARADISE_API AWPSoldierCharacter : public ACharacter\n"
            "{\n GENERATED_BODY()\n};"
        )
        self.assertEqual(
            audit.reflected_declarations(header, "UCLASS"),
            ["AWPSoldierCharacter"],
        )

    def test_dynamic_and_unique_dynamic_bindings_are_both_checked(self) -> None:
        source = """
            OnDowned.AddDynamic(this, &AWPSoldierCharacter::HandleDowned);
            OnHealthChanged.AddUniqueDynamic(
                this, &AWPSoldierCharacter::HandleHealthChanged);
        """
        self.assertEqual(
            audit.ADD_DYNAMIC_RE.findall(source),
            [
                ("AWPSoldierCharacter", "HandleDowned"),
                ("AWPSoldierCharacter", "HandleHealthChanged"),
            ],
        )

    def test_reflected_handler_requires_ufunction(self) -> None:
        good = (
            "UFUNCTION()\n"
            "void HandleDowned();\n"
            "void HandleUnreflected();"
        )
        self.assertTrue(audit.has_reflected_handler(good, "HandleDowned"))
        self.assertFalse(
            audit.has_reflected_handler(good, "HandleUnreflected")
        )

    def test_bound_method_with_cpp_definition_passes(self) -> None:
        header = "UFUNCTION() void HandleDowned();"
        cpp = {
            Path("soldier.cpp"):
                "void AWPSoldierCharacter::HandleDowned() { StopFire(); }"
        }
        self.assertTrue(
            audit.has_handler_implementation(
                "AWPSoldierCharacter", "HandleDowned", header, cpp
            )
        )

    def test_bound_method_missing_cpp_definition_fails(self) -> None:
        header = "UFUNCTION() void HandleDowned();"
        cpp = {
            Path("soldier.cpp"):
                "void AWPSoldierCharacter::HandleRevived() {}"
        }
        self.assertFalse(
            audit.has_handler_implementation(
                "AWPSoldierCharacter", "HandleDowned", header, cpp
            )
        )

    def test_wrong_class_method_is_not_a_valid_implementation(self) -> None:
        header = "UFUNCTION() void HandleDowned();"
        cpp = {
            Path("other.cpp"):
                "void AWPOtherCharacter::HandleDowned() {}"
        }
        self.assertFalse(
            audit.has_handler_implementation(
                "AWPSoldierCharacter", "HandleDowned", header, cpp
            )
        )

    def test_inline_ufunction_implementation_passes(self) -> None:
        header = """
            UFUNCTION()
            bool IsCombatReady() const { return true; }
        """
        self.assertTrue(
            audit.has_handler_implementation(
                "AWPSoldierCharacter", "IsCombatReady", header, {}
            )
        )

    def test_only_declaration_is_not_treated_as_inline_definition(self) -> None:
        header = "UFUNCTION() bool IsCombatReady() const;"
        self.assertFalse(
            audit.has_handler_implementation(
                "AWPSoldierCharacter", "IsCombatReady", header, {}
            )
        )


if __name__ == "__main__":
    unittest.main()
