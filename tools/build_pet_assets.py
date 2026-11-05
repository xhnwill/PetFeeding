"""Compile approved PNG art into shared 96px web and indexed RGB565 firmware assets.
This is an asset-format conversion step; original generated art is preserved.
Requires Pillow. No API credentials or online access.
"""
from pathlib import Path
import json
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]
PETS = [
    ("forest", "芽芽", "森林小兽", "叶片耳朵、头顶嫩芽、叶尾", "温柔，好奇，喜欢晒太阳"),
    ("nebula", "团团", "星云小兽", "月牙额纹、小翼、星星尾巴", "慢悠悠，爱做梦，喜欢看星星"),
    ("sunshine", "小麦", "阳光小兽", "太阳鬃毛、短角、卷卷尾", "开朗，勇敢，喜欢玩球"),
]
SIZE = 96

def compile_assets():
    out = ROOT / "web/assets/pets"
    out.mkdir(parents=True, exist_ok=True)
    manifest, definitions = [], []
    for species, (slug, name, title, silhouette, personality) in enumerate(PETS):
        source = ROOT / f"assets/pets/source/{slug}.png"
        with Image.open(source) as original:
            if original.mode != "RGBA" or original.getextrema()[3][0] != 0:
                raise ValueError(f"{slug}: a transparent RGBA original is required")
            image = original.resize((SIZE, SIZE), Image.Resampling.NEAREST)
        alpha = image.getchannel("A")
        indexed = image.convert("RGB").quantize(colors=31, method=Image.Quantize.FASTOCTREE)
        raw_palette = indexed.getpalette()
        palette = [(0, 0, 0)] + [tuple(raw_palette[i*3:i*3+3]) for i in range(31)]
        indexes = [value+1 if a>=128 else 0 for value, a in zip(indexed.getdata(), alpha.getdata())]
        # Web image uses exactly the same palette, opacity threshold and pixels as firmware.
        web = Image.new("RGBA", (SIZE, SIZE))
        web.putdata([(*palette[i], 255) if i else (0, 0, 0, 0) for i in indexes])
        web.save(out / f"{slug}.png", optimize=True)
        colors = [((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in palette]
        # On-wire high-byte first, same pixel format as board.c's strip buffer.
        colors = [((c>>8)|(c<<8)) & 0xffff for c in colors]
        lines = ["  "+",".join(map(str,indexes[y*SIZE:(y+1)*SIZE])) for y in range(SIZE)]
        definitions.append(f"static const uint8_t sprite_{slug}_pixels[PET_ART_SIZE*PET_ART_SIZE]={{\n"+",\n".join(lines)+"\n};")
        definitions.append(f"static const uint16_t sprite_{slug}_palette[32]={{"+",".join(hex(c) for c in colors)+"};")
        manifest.append(dict(id=slug, species=species, name=name, title=title,
                             silhouette=silhouette, personality=personality,
                             width=SIZE, height=SIZE, src=f"./assets/pets/{slug}.png"))
    header = "#pragma once\n#include <stdint.h>\n#define PET_ART_SIZE 96\n"
    header += "\n".join(definitions)
    header += "\nstatic const uint8_t *const pet_art_pixels[3]={" + ",".join(f"sprite_{p[0]}_pixels" for p in PETS) + "};\n"
    header += "static const uint16_t *const pet_art_palettes[3]={" + ",".join(f"sprite_{p[0]}_palette" for p in PETS) + "};\n"
    (ROOT/"firmware/main/pet_art.h").write_text(header, encoding="utf-8")
    (out/"manifest.json").write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print("Compiled 3 transparent 96x96 sprites; 27,840 bytes of firmware pixels and palettes.")
if __name__ == "__main__":
    compile_assets()
