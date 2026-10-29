"""Generate the firmware UI subset from OFL-licensed Noto Sans SC.
The generated glyph bitmap is distributed with FONT-LICENSE.txt."""
from pathlib import Path
from PIL import Image, ImageFont, ImageDraw
import re
root=Path(__file__).resolve().parents[1]
text="".join(p.read_text(encoding="utf-8") for p in (root/"firmware/main").glob("*.c"))
chars=sorted(set(chr(i) for i in range(32,127))|set(re.findall(r"[\u4e00-\u9fff]",text)))
font_path=root/"tools/NotoSansSC.ttf"
if not font_path.exists():
    raise SystemExit("Place an OFL-licensed NotoSansSC.ttf in tools/ before generating")
font=ImageFont.truetype(str(font_path),16)
rows=[]
for ch in chars:
    im=Image.new("1",(16,18));ImageDraw.Draw(im).text((0,-1),ch,font=font,fill=1)
    rows.append("{" + ",".join(str(sum(1<<(15-x) for x in range(16) if im.getpixel((x,y)))) for y in range(18))+"}")
out="#pragma once\n#include <stdint.h>\n#define FONT_COUNT "+str(len(chars))+"\n"
out+="static const uint32_t font_codes[]={"+",".join(str(ord(c)) for c in chars)+"};\n"
out+="static const uint16_t font_bits[][18]={"+",\n".join(rows)+"};\n"
(root/"firmware/main/font.h").write_text(out,encoding="utf-8")
print(f"{len(chars)} glyphs")
