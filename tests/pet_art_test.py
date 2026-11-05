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

    def test_motion_atlases_and_packed_device_frames(self):
        manifest = json.loads((ROOT/"web/assets/pets/manifest.json").read_text(encoding="utf-8"))
        header = (ROOT/"firmware/main/pet_motion.h").read_text(encoding="utf-8")
        for pet in manifest:
            slug = pet["id"]
            with Image.open(ROOT/f"web/assets/pets/{slug}-actions.png") as atlas:
                self.assertEqual(atlas.size, (384,288))
                for row, action in enumerate(["sleep","eat","play"]):
                    animation = pet["motions"][action]
                    self.assertEqual(animation["frames"],4)
                    self.assertEqual(animation["row"],row)
                    palette = [int(v,16) for v in re.search(rf"motion_{slug}_{action}_palette[^=]*=\{{(.*?)\}};",header,re.S)[1].split(",")]
                    self.assertEqual(len(palette),16)
                    unique = set()
                    for frame in range(4):
                        with self.subTest(slug=slug,action=action,frame=frame):
                            rgba = list(atlas.crop((frame*96,row*96,(frame+1)*96,(row+1)*96)).getdata())
                            self.assertLessEqual(len(set(rgba)),16)
                            packed = [int(v) for v in re.search(rf"motion_{slug}_{action}_{frame}\[[^=]*=\{{(.*?)\}};",header,re.S)[1].split(",")]
                            self.assertEqual(len(packed),96*96//2)
                            unique.add(bytes(packed))
                            if action=="play":
                                ball = atlas.crop((frame*96,row*96+68,frame*96+30,(row+1)*96))
                                warm = sum(a>0 and r>120 and r>g*1.3 and r>b*1.2 for r,g,b,a in ball.getdata())
                                self.assertGreater(warm,4,"the red play ball must not be lost to body-color quantization")
                            indexes = [value for b in packed for value in (b>>4,b&15)]
                            for (r,g,b,a),index in zip(rgba,indexes):
                                self.assertEqual(index==0,a==0)
                                if a:
                                    rgb565=((r>>3)<<11)|((g>>2)<<5)|(b>>3)
                                    self.assertEqual(palette[index],((rgb565>>8)|(rgb565<<8))&65535)
                    self.assertGreaterEqual(len(unique),3,"animation must contain real pose changes")
                    with Image.open(ROOT/f"web/assets/pets/{slug}-{action}.gif") as preview:
                        self.assertEqual(preview.n_frames,4)
                        self.assertEqual(preview.info["duration"],animation["frameMs"])


if __name__ == "__main__":
    unittest.main()
