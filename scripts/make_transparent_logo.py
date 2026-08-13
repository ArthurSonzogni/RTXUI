#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Make outer background of RTXUI logo images transparent (PNG with Alpha).

Creates smooth antialiased rounded-rectangle masks around the inner cards in
docs/public/logo-dark.png and docs/public/logo-light.png, removing any outer
background mismatch with VitePress theme backgrounds.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
PUBLIC_DIR = ROOT / "docs" / "public"


def process_logo(input_path: Path, output_path: Path, padding=20):
    img = Image.open(input_path).convert("RGBA")
    w, h = img.size
    
    # 1. Find bounding box of card pixels
    # Sample background color near corners
    corner_bg = img.getpixel((5, 5))[:3]
    
    def color_diff(c1, c2):
        return abs(c1[0]-c2[0]) + abs(c1[1]-c2[1]) + abs(c1[2]-c2[2])
    
    min_x, max_x = w, 0
    min_y, max_y = h, 0
    
    for y in range(0, h, 2):
        for x in range(0, w, 2):
            if color_diff(img.getpixel((x, y))[:3], corner_bg) > 20:
                if x < min_x: min_x = x
                if x > max_x: max_x = x
                if y < min_y: min_y = y
                if y > max_y: max_y = y

    # Calculate card dimensions
    card_w = max_x - min_x
    card_h = max_y - min_y
    
    print(f"[{input_path.name}] Detected card bbox: ({min_x}, {min_y}) to ({max_x}, {max_y}), size: {card_w}x{card_h}")
    
    # Standardize bounding box to a square
    size = max(card_w, card_h)
    center_x = (min_x + max_x) // 2
    center_y = (min_y + max_y) // 2
    
    box_left = max(0, center_x - size // 2)
    box_top = max(0, center_y - size // 2)
    box_right = min(w, center_x + size // 2)
    box_bottom = min(h, center_y + size // 2)
    
    # Crop to card box
    cropped = img.crop((box_left, box_top, box_right, box_bottom))
    cw, ch = cropped.size

    # Create high-res mask for antialiasing
    scale = 4
    mask_size = (cw * scale, ch * scale)
    mask = Image.new("L", mask_size, 0)
    draw = ImageDraw.Draw(mask)
    
    radius = int(cw * scale * 0.18)  # ~18% corner radius matching iOS/macOS icon style
    draw.rounded_rectangle([0, 0, mask_size[0], mask_size[1]], radius=radius, fill=255)
    
    # Downsample mask for smooth antialiased edges
    mask = mask.resize((cw, ch), resample=Image.Resampling.LANCZOS)
    
    # Apply alpha mask
    cropped.putalpha(mask)
    
    # Save as PNG
    cropped.save(output_path, "PNG")
    print(f"Saved transparent logo to {output_path} (size: {cw}x{ch})")


def main():
    dark_path = PUBLIC_DIR / "logo-dark.png"
    light_path = PUBLIC_DIR / "logo-light.png"
    
    process_logo(dark_path, dark_path)
    process_logo(light_path, light_path)
    
    # Also update default fallback logo.png
    fallback_path = PUBLIC_DIR / "logo.png"
    process_logo(dark_path, fallback_path)


if __name__ == "__main__":
    main()
