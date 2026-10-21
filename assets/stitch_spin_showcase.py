import os
from PIL import Image, ImageDraw, ImageFont

asset_dir = r"D:\Game_prj\game_engine\assets"

angles_info = [
    (0,   "0° (Face-On / Front)",             "spin_frame_000.png"),
    (45,  "45° (CW Turn Right)",              "spin_frame_045.png"),
    (90,  "90° (Edge-On / Concealed Profile)","spin_frame_090.png"),
    (135, "135° (Quarter-Rear)",              "spin_frame_135.png"),
    (180, "180° (Rear Face / Friend-Foe)",    "spin_frame_180.png"),
    (225, "225° (Quarter-Rear Left)",         "spin_frame_225.png"),
    (270, "270° (Edge-On Left Profile)",      "spin_frame_270.png"),
    (315, "315° (Quarter-Front Left)",        "spin_frame_315.png")
]

# Layout: 4 columns x 2 rows
cols = 4
rows = 2
cell_w = 400
cell_h = 500
label_h = 45
header_h = 80
margin = 15

img_w = cols * cell_w + (cols + 1) * margin
img_h = header_h + rows * (cell_h + label_h) + (rows + 1) * margin

canvas = Image.new("RGBA", (img_w, img_h), (22, 24, 28, 255))
draw = ImageDraw.Draw(canvas)

# Fonts
font_title = None
font_sub = None
font_cell = None

try:
    font_title = ImageFont.truetype("arialbd.ttf", 26)
    font_sub   = ImageFont.truetype("arial.ttf", 15)
    font_cell  = ImageFont.truetype("arialbd.ttf", 15)
except:
    font_title = ImageFont.load_default()
    font_sub   = ImageFont.load_default()
    font_cell  = ImageFont.load_default()

# Draw Header
title_text = "VINASHOOT TARGET - 360° CIRCULAR ROTATION (Y-AXIS SHAFT)"
sub_text   = "Continuous circular rotation around vertical cylindrical axis passing from top crown straight down into base"
draw.text((margin + 10, margin), title_text, fill=(230, 235, 240, 255), font=font_title)
draw.text((margin + 10, margin + 35), sub_text, fill=(160, 175, 190, 255), font=font_sub)

# Draw subtle accent line
draw.line([(margin, header_h - 10), (img_w - margin, header_h - 10)], fill=(60, 80, 110, 255), width=2)

for idx, (ang, label, fn) in enumerate(angles_info):
    c = idx % cols
    r = idx // cols
    
    x0 = margin + c * (cell_w + margin)
    y0 = header_h + margin + r * (cell_h + label_h + margin)
    
    # Cell background card
    draw.rounded_rectangle([x0, y0, x0 + cell_w, y0 + cell_h + label_h], radius=8, fill=(30, 33, 40, 255), outline=(50, 56, 68, 255), width=1)
    
    # Load and resize frame
    frame_p = os.path.join(asset_dir, fn)
    if os.path.exists(frame_p):
        f_im = Image.open(frame_p)
        # Resize to fit within cell_w x cell_h maintaining aspect ratio
        f_im.thumbnail((cell_w - 10, cell_h - 10), Image.Resampling.LANCZOS)
        
        # Center in frame area
        paste_x = x0 + (cell_w - f_im.width) // 2
        paste_y = y0 + 5 + (cell_h - f_im.height) // 2
        
        canvas.alpha_composite(f_im.convert("RGBA"), (paste_x, paste_y))
    
    # Label bar at bottom of card
    bar_y0 = y0 + cell_h
    draw.rounded_rectangle([x0 + 4, bar_y0, x0 + cell_w - 4, bar_y0 + label_h - 4], radius=4, fill=(40, 45, 55, 255))
    
    # Angle badge (colored pill)
    badge_w = 48
    draw.rounded_rectangle([x0 + 10, bar_y0 + 7, x0 + 10 + badge_w, bar_y0 + label_h - 11], radius=3, fill=(180, 30, 30, 255) if ang == 0 else (45, 100, 180, 255))
    draw.text((x0 + 14, bar_y0 + 12), f"{ang:3d}°", fill=(255, 255, 255, 255), font=font_cell)
    
    # Description text
    desc = label.split("(")[-1].rstrip(")") if "(" in label else label
    draw.text((x0 + 10 + badge_w + 10, bar_y0 + 12), desc, fill=(220, 225, 230, 255), font=font_cell)

out_showcase = os.path.join(asset_dir, "preview_spin_showcase.png")
canvas.convert("RGB").save(out_showcase, quality=95)
print(f"Successfully generated spin showcase: {out_showcase} ({img_w}x{img_h})")
