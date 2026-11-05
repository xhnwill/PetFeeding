"""Compile approved PNG art into shared 96px web and indexed RGB565 firmware assets.
This is an asset-format conversion step; original generated art is preserved.
Requires Pillow. No API credentials or online access.
"""
from pathlib import Path
import json
from PIL import Image, ImageDraw, ImageFont
ROOT = Path(__file__).resolve().parents[1]
PETS = [
    ("forest", "芽芽", "森林小兽", "叶片耳朵、头顶嫩芽、叶尾", "温柔，好奇，喜欢晒太阳"),
    ("nebula", "团团", "星云小兽", "月牙额纹、小翼、星星尾巴", "慢悠悠，爱做梦，喜欢看星星"),
    ("sunshine", "小麦", "阳光小兽", "太阳鬃毛、短角、卷卷尾", "开朗，勇敢，喜欢玩球"),
]
SIZE = 96
MOTIONS = [("sleep", 700), ("eat", 220), ("play", 180)]


def compile_motions(manifest):
    """Slice equal sheet cells, preserving the source's shared canvas and pivot."""
    out = ROOT / "web/assets/pets"
    definitions, previews = [], {}
    for species, (slug, *_) in enumerate(PETS):
        source = ROOT / f"assets/pets/source/{slug}-actions.png"
        with Image.open(source) as original:
            if original.mode != "RGBA" or original.getextrema()[3][0] != 0:
                raise ValueError(f"{slug}: transparent RGBA motion sheet required")
            if original.width % 4 or original.height % 3:
                raise ValueError(f"{slug}: source must divide into 4 columns and 3 rows")
            w, h = original.width//4, original.height//3
            if w != h:
                raise ValueError(f"{slug}: square source cells required, got {w}x{h}")
            frames = [[original.crop((col*w,row*h,(col+1)*w,(row+1)*h)).resize((SIZE,SIZE),Image.Resampling.NEAREST)
                       for col in range(4)] for row in range(3)]
        atlas = Image.new("RGBA", (SIZE*4, SIZE*3))
        manifest[species]["motions"] = {}
        for row, (action, duration) in enumerate(MOTIONS):
            # One shared 15-color palette for each 4-frame loop prevents palette flicker.
            strip = Image.new("RGBA", (SIZE*4, SIZE))
            for col in range(4):
                strip.paste(frames[row][col], (col*SIZE, 0))
            # Transparent RGB noise must not consume scarce palette entries.
            visible = [(r,g,b) for r,g,b,a in strip.getdata() if a>=128]
            palette_source = Image.new("RGB",(len(visible),1));palette_source.putdata(visible)
            # Preserve the small red ball and gold star instead of merging them into body colors.
            protected=[]
            for match in [lambda r,g,b:r>120 and r>g*1.35 and r>b*1.25,
                          lambda r,g,b:r>150 and g>100 and b<g*.75]:
                samples=[rgb for rgb in visible if match(*rgb)]
                if samples:protected.append(tuple(sum(v[i] for v in samples)//len(samples) for i in range(3)))
            count=15-len(protected)
            selected = palette_source.quantize(colors=count,method=Image.Quantize.MEDIANCUT)
            palette_image=Image.new("P",(1,1))
            palette_image.putpalette(selected.getpalette()[:count*3]+[v for rgb in protected for v in rgb])
            quantized = strip.convert("RGB").quantize(palette=palette_image,dither=Image.Dither.NONE)
            raw = quantized.getpalette()
            palette = [(0,0,0)] + [tuple(raw[i*3:i*3+3]) for i in range(15)]
            colors = [((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in palette]
            colors = [((c>>8)|(c<<8)) & 65535 for c in colors]
            definitions.append(f"static const uint16_t motion_{slug}_{action}_palette[16]={{"+",".join(hex(c) for c in colors)+"};")
            gif_frames, rgba_frames = [], []
            for col in range(4):
                q = quantized.crop((col*SIZE,0,(col+1)*SIZE,SIZE))
                alpha = frames[row][col].getchannel("A")
                indexes = [v+1 if a>=128 else 0 for v,a in zip(q.getdata(),alpha.getdata())]
                rgba = Image.new("RGBA",(SIZE,SIZE))
                rgba.putdata([(*palette[i],255) if i else (0,0,0,0) for i in indexes])
                atlas.paste(rgba,(col*SIZE,row*SIZE))
                rgba_frames.append(rgba)
                packed = [(indexes[i]<<4)|indexes[i+1] for i in range(0,len(indexes),2)]
                lines = ["  "+",".join(map(str,packed[y*SIZE//2:(y+1)*SIZE//2])) for y in range(SIZE)]
                definitions.append(f"static const uint8_t motion_{slug}_{action}_{col}[PET_ART_SIZE*PET_ART_SIZE/2]={{\n"+",\n".join(lines)+"\n};")
                gif = Image.new("P",(SIZE,SIZE));gif.putdata(indexes)
                gif.putpalette([v for color in palette for v in color]+[0]*(768-48))
                gif.info["transparency"]=0
                gif_frames.append(gif.resize((288,288),Image.Resampling.NEAREST))
            gif_frames[0].save(out/f"{slug}-{action}.gif",save_all=True,append_images=gif_frames[1:],
                               duration=duration,loop=0,transparency=0,disposal=2,optimize=False)
            previews[(slug, action)] = rgba_frames
            manifest[species]["motions"][action] = dict(src=f"./assets/pets/{slug}-actions.png",row=row,
                                                        frames=4,frameMs=duration,preview=f"./assets/pets/{slug}-{action}.gif")
        atlas.save(out/f"{slug}-actions.png",optimize=True)
    header = "#pragma once\n#include <stdint.h>\n"+"\n".join(definitions)
    header += "\nstatic const uint8_t *const pet_motion_pixels[3][3][4]={\n"
    for slug, *_ in PETS:
        header += "{"+",".join("{"+",".join(f"motion_{slug}_{action}_{i}" for i in range(4))+"}" for action,_ in MOTIONS)+"},\n"
    header += "};\nstatic const uint16_t *const pet_motion_palettes[3][3]={\n"
    for slug, *_ in PETS:
        header += "{"+",".join(f"motion_{slug}_{action}_palette" for action,_ in MOTIONS)+"},\n"
    header += "};\n"
    (ROOT/"firmware/main/pet_motion.h").write_text(header,encoding="utf-8")
    # Shareable animated review sheet. Its common 400ms frame cadence is for comparison.
    font_path = ROOT/"tools/NotoSansSC.ttf"
    font = ImageFont.truetype(str(font_path),22) if font_path.exists() else ImageFont.load_default()
    has_cjk = font_path.exists()
    overview=[]
    for frame in range(4):
        page=Image.new("RGB",(660,720),(248,245,236));draw=ImageDraw.Draw(page)
        for col,title in enumerate(["睡觉","吃饭","玩耍"] if has_cjk else ["Sleep","Eat","Play"]):
            draw.text((110+col*210,12),title,font=font,fill=(45,66,55))
        for row,(slug,name,*_) in enumerate(PETS):
            draw.text((16,60+row*220),name if has_cjk else slug,font=font,fill=(45,66,55))
            for col,(action,_) in enumerate(MOTIONS):
                im=previews[(slug,action)][frame].resize((180,180),Image.Resampling.NEAREST)
                page.paste(im,(28+col*210,80+row*210),im)
        overview.append(page)
    overview[0].save(ROOT/"docs/pet-actions-v0.4.0.gif",save_all=True,append_images=overview[1:],
                     duration=400,loop=0,optimize=False)
    print("Compiled 36 action frames, 9 transparent animated GIFs, 3 atlases and 4bpp firmware data.")

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
    compile_motions(manifest)
    (out/"manifest.json").write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print("Compiled 3 transparent 96x96 sprites; 27,840 bytes of firmware pixels and palettes.")
if __name__ == "__main__":
    compile_assets()
