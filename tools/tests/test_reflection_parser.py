"""Regression tests for nested Unreal reflection metadata in static audit."""
import unittest
from tools.check_compile_readiness import reflected_declarations


class ReflectionParserTests(unittest.TestCase):
    def test_nested_uclass_properties(self):
        code = (
            "UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))\n"
            "class WARRIORPARADISE_API UWPHealthComponent : public UActorComponent { GENERATED_BODY() };"
        )
        self.assertEqual(reflected_declarations(code, "UCLASS"), ["UWPHealthComponent"])

    def test_metadata_lists_and_multiple_classes(self):
        code = (
            "UCLASS(meta=(BlueprintSpawnableComponent, DisplayName=\"Example\"))\n"
            "class UWPExample : public UActorComponent {};\n"
            "UCLASS(Blueprintable) class AWPSoldierCharacter : public ACharacter {};"
        )
        self.assertEqual(
            reflected_declarations(code, "UCLASS"),
            ["UWPExample", "AWPSoldierCharacter"],
        )

    def test_unterminated_and_missing_class(self):
        self.assertEqual(reflected_declarations("UCLASS(meta=(Broken)", "UCLASS"), [])


if __name__ == "__main__":
    unittest.main()
