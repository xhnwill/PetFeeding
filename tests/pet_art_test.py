"""Check actual web pixels against compiled device palettes and transparency."""
import json
import re
import unittest
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


class PetArtTest(unittest.TestCase):
    def test_shared_pixels_and_transparency(self):
        manifest = json.loads((ROOT / "web/assets/pets/manifest.json").read_text(encoding="utf-8"))
        self.assertEqual([p["species"] for p in manifest], [0, 1, 2])
        header = (ROOT / "firmware/main/pet_art.h").read_text(encoding="utf-8")
        masks = []
        for pet in manifest:
            slug = pet["id"]
            with self.subTest(slug=slug), Image.open(ROOT / f"web/assets/pets/{slug}.png") as image:
                self.assertEqual(image.size, (96, 96))
                self.assertEqual(image.mode, "RGBA")
                pixels = list(image.getdata())
                self.assertLessEqual(len(set(pixels)), 32)
                self.assertEqual(set(a for r, g, b, a in pixels), {0, 255})
                indexes = [int(v) for v in re.search(rf"sprite_{slug}_pixels[^=]*=\{{(.*?)\}};", header, re.S)[1].split(",")]
                palette = [int(v, 16) for v in re.search(rf"sprite_{slug}_palette[^=]*=\{{(.*?)\}};", header, re.S)[1].split(",")]
                self.assertEqual(len(indexes), 96 * 96)
                self.assertEqual(len(palette), 32)
                for (r, g, b, a), index in zip(pixels, indexes):
                    self.assertTrue(0 <= index < 32)
                    self.assertEqual(index == 0, a == 0)
                    if a:
                        rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
                        self.assertEqual(palette[index], ((rgb565 >> 8) | (rgb565 << 8)) & 65535)
                masks.append(tuple(a for r, g, b, a in pixels))
        self.assertEqual(len(set(masks)), 3, "species must have different silhouettes")


if __name__ == "__main__":
    unittest.main()
