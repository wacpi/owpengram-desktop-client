#!/usr/bin/env python3
"""Generate all branding icons from source files"""
import os
import sys
from pathlib import Path
from PIL import Image

def resize_png(input_path, output_path, size, quality=95):
    """Resize PNG image to specified size"""
    try:
        img = Image.open(input_path)
        img = img.resize(size, Image.Resampling.LANCZOS)
        img.save(output_path, 'PNG', optimize=True)
        print(f"Created {output_path} ({size[0]}x{size[1]})")
        return True
    except Exception as e:
        print(f"Error creating {output_path}: {e}")
        return False

def create_ico(input_path, output_path):
    """Create ICO file with multiple sizes from PNG"""
    try:
        img = Image.open(input_path)
        sizes = [(256, 256), (128, 128), (96, 96), (64, 64), (48, 48), (32, 32), (16, 16)]
        
        ico_images = []
        for size in sizes:
            resized = img.resize(size, Image.Resampling.LANCZOS)
            ico_images.append(resized)
        
        ico_images[0].save(
            output_path,
            format='ICO',
            sizes=[(img.width, img.height) for img in ico_images],
            append_images=ico_images[1:]
        )
        print(f"Created {output_path} with sizes: {[s[0] for s in sizes]}")
        return True
    except Exception as e:
        print(f"Error creating {output_path}: {e}")
        return False

def create_svg_variant(input_svg_path, output_svg_path, fill_color=None, opacity=None):
    """Create SVG variant with different color or opacity"""
    try:
        with open(input_svg_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        if fill_color:
            content = content.replace('fill="#ffffff"', f'fill="{fill_color}"')
            content = content.replace('fill="#9e9e9e"', f'fill="{fill_color}"')
        
        if opacity is not None:
            if '<g' in content and 'opacity' not in content:
                content = content.replace('<g transform=', f'<g opacity="{opacity}" transform=')
            elif 'opacity=' in content:
                import re
                content = re.sub(r'opacity="[^"]*"', f'opacity="{opacity}"', content)
            else:
                content = content.replace('<svg', f'<g opacity="{opacity}"><svg').replace('</svg>', '</g></svg>')
        
        with open(output_svg_path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Created {output_svg_path}")
        return True
    except Exception as e:
        print(f"Error creating {output_svg_path}: {e}")
        return False

def main():
    script_dir = Path(__file__).parent
    icons_dir = script_dir / 'icons'
    art_dir = script_dir / 'art'
    
    art_dir.mkdir(exist_ok=True)
    
    logo_1024 = icons_dir / 'logo_1024.png'
    tray_svg = icons_dir / 'tray_monochrome.svg'
    
    if not logo_1024.exists():
        print(f"Error: {logo_1024} not found")
        return 1
    
    if not tray_svg.exists():
        print(f"Error: {tray_svg} not found")
        return 1
    
    print("Generating PNG icons...")
    
    resize_png(logo_1024, art_dir / 'logo_256.png', (256, 256))
    resize_png(logo_1024, art_dir / 'logo_256_no_margin.png', (256, 256))
    resize_png(logo_1024, art_dir / 'business_logo.png', (256, 256))
    resize_png(logo_1024, art_dir / 'affiliate_logo.png', (256, 256))
    
    print("\nGenerating Linux icons...")
    linux_sizes = [
        (16, 16, 32, 32),
        (32, 32, 64, 64),
        (48, 48, 96, 96),
        (64, 64, 128, 128),
        (128, 128, 256, 256),
        (256, 256, 512, 512),
        (512, 512, 1024, 1024),
    ]
    
    for size1x, size1y, size2x, size2y in linux_sizes:
        resize_png(logo_1024, art_dir / f'icon{size1x}.png', (size1x, size1y))
        resize_png(logo_1024, art_dir / f'icon{size1x}@2x.png', (size2x, size2y))

    print("\nGenerating ICO file...")
    create_ico(logo_1024, art_dir / 'icon256.ico')

    logo_debug_1024 = icons_dir / 'logo_debug_1024.png'
    if logo_debug_1024.exists():
        print("\nGenerating Linux debug icons...")
        for size1x, size1y, size2x, size2y in linux_sizes:
            resize_png(logo_debug_1024, art_dir / f'icon{size1x}_debug.png', (size1x, size1y))
            resize_png(logo_debug_1024, art_dir / f'icon{size1x}_debug@2x.png', (size2x, size2y))
    else:
        print(f"\nSkipping Linux debug icons: {logo_debug_1024} not found")
    
    print("\nGenerating SVG variants...")
    create_svg_variant(tray_svg, icons_dir / 'plane_white.svg', fill_color='#ffffff')
    
    gray_color = '#9e9e9e'
    create_svg_variant(tray_svg, icons_dir / 'tray_monochrome.svg', fill_color=gray_color)
    create_svg_variant(tray_svg, icons_dir / 'tray_monochrome_attention.svg', fill_color='#fc8c03')
    create_svg_variant(tray_svg, icons_dir / 'tray_monochrome_mute.svg', fill_color=gray_color, opacity=0.5)
    
    print("\nAll icons generated successfully!")
    return 0

if __name__ == "__main__":
    sys.exit(main())
