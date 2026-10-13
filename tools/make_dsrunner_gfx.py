"""Generates DsRunner theme tile sprites with rounded corners and glowing borders.
Outputs 64x64 4-bpp indexed PNGs compatible with grit.
"""
import os
import math
from PIL import Image, ImageDraw, ImageFilter

W, H = 64, 64
BODY = 40      # tile body size in px
BODY_X = 12    # body offset in the 64x64 sprite; drawn at cell position - 10
RADIUS = 7

# DsRunner dark palette
BG_DARK = (15, 17, 23)        # #0F1117
TILE_COLOR = (35, 40, 56)     # #232838
TILE_DIM = (30, 34, 48)       # Dimmed 25% with BG
ACCENT = (76, 194, 255)       # #4CC2FF
WHITE = (255, 255, 255)

def make_unselected_tile():
    """Generates 48x48 rounded tile centered in 64x64 with blend weights in R & G."""
    # Scale up for high-quality supersampled anti-aliasing
    SCALE = 8
    sw, sh = W * SCALE, H * SCALE
    im = Image.new("L", (sw, sh), 0)
    d = ImageDraw.Draw(im)
    
    # 40x40 body centered in 64x64 (12..51). The grid cell is 44x44 with 4px spacing,
    # so the body leaves 2px inside the cell plus 4px spacing for the selection glow.
    x0, y0 = BODY_X * SCALE, BODY_X * SCALE
    x1, y1 = (BODY_X + BODY) * SCALE - 1, (BODY_X + BODY) * SCALE - 1
    radius = RADIUS * SCALE
    d.rounded_rectangle((x0, y0, x1, y1), radius, fill=255)
    
    # Downsample
    mask = im.resize((W, H), Image.LANCZOS)
    
    # Create indexed image with 16-level blend factors
    # Index 0: 0% card (transparent / background)
    # Index 15: 100% card
    out = Image.new("P", (W, H), 0)
    palette = []
    for i in range(16):
        # r = background weight (31..0), g = card weight (0..31)
        card_weight = int(i * 31 / 15 + 0.5)
        bg_weight = 31 - card_weight
        # We store as 8-bit RGB in PNG palette:
        palette.extend([bg_weight * 8, card_weight * 8, 0])
    while len(palette) < 768:
        palette.extend([0, 0, 0])
    out.putpalette(palette)
    
    # Map mask values (0..255) to palette index (0..15)
    px = out.load()
    mpx = mask.load()
    for y in range(H):
        for x in range(W):
            v = mpx[x, y]
            idx = int(v * 15 / 255 + 0.5)
            px[x, y] = idx
            
    return out

def make_selected_tile():
    """Generates 48x48 rounded tile with cyan glow border and inner white stroke."""
    SCALE = 8
    sw, sh = W * SCALE, H * SCALE
    
    # We create high-res RGBA image
    base = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    
    x0, y0 = BODY_X * SCALE, BODY_X * SCALE
    x1, y1 = (BODY_X + BODY) * SCALE - 1, (BODY_X + BODY) * SCALE - 1
    radius = RADIUS * SCALE
    
    # 1. Glow border (expanded by 3px with blur)
    glow = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow)
    gd.rounded_rectangle((x0 - 3 * SCALE, y0 - 3 * SCALE, x1 + 3 * SCALE, y1 + 3 * SCALE), radius + 3 * SCALE, fill=(*ACCENT, 180))
    glow = glow.filter(ImageFilter.GaussianBlur(2.5 * SCALE))
    base.paste(glow, (0, 0), glow)
    
    # 2. Cyan border (width 2px)
    bd = ImageDraw.Draw(base)
    bd.rounded_rectangle((x0 - 2 * SCALE, y0 - 2 * SCALE, x1 + 2 * SCALE, y1 + 2 * SCALE), radius + 2 * SCALE, outline=(*ACCENT, 255), width=2 * SCALE)
    
    # 3. White highlight stroke (width 1px)
    bd.rounded_rectangle((x0 - 1 * SCALE, y0 - 1 * SCALE, x1 + 1 * SCALE, y1 + 1 * SCALE), radius + 1 * SCALE, outline=(*WHITE, 255), width=1 * SCALE)
    
    # 4. Tile fill
    bd.rounded_rectangle((x0, y0, x1, y1), radius, fill=(*TILE_COLOR, 255))
    
    # Downscale with LANCZOS to 64x64
    small = base.resize((W, H), Image.LANCZOS)
    
    # Composite over BG_DARK to get final colors for quantization
    bg = Image.new("RGB", (W, H), BG_DARK)
    bg.paste(small, (0, 0), small)
    
    # Quantize to 16 colors (index 0 reserved for transparent background)
    # Let's extract transparent mask: pixels where small alpha == 0
    alpha = small.split()[3]
    
    # Quantize RGB to 15 colors
    q = bg.quantize(colors=15, method=Image.MEDIANCUT)
    q_pal = q.getpalette()[:45] # 15 colors * 3
    
    # Final 16-color image: index 0 is transparent (BG_DARK)
    final_pal = list(BG_DARK) + q_pal
    while len(final_pal) < 768:
        final_pal.extend([0, 0, 0])
        
    out = Image.new("P", (W, H), 0)
    out.putpalette(final_pal)
    opx = out.load()
    qpx = q.load()
    apx = alpha.load()
    
    for y in range(H):
        for x in range(W):
            if apx[x, y] < 8:
                opx[x, y] = 0
            else:
                opx[x, y] = qpx[x, y] + 1 # shift indices by 1
                
    return out

if __name__ == "__main__":
    out_dir = "arm9/gfx"
    os.makedirs(out_dir, exist_ok=True)
    
    unsel = make_unselected_tile()
    unsel.save(os.path.join(out_dir, "dsrunnerTile.png"))
    
    sel = make_selected_tile()
    sel.save(os.path.join(out_dir, "dsrunnerTileSelected.png"))
    
    # grit rule files: tile format, 4bpp
    grit_content = "# tile format\n-gt\n# 16 colors\n-gB4\n"
    with open(os.path.join(out_dir, "dsrunnerTile.grit"), "w") as f:
        f.write(grit_content)
    with open(os.path.join(out_dir, "dsrunnerTileSelected.grit"), "w") as f:
        f.write(grit_content)
        
    print("dsrunnerTile and dsrunnerTileSelected generated!")
