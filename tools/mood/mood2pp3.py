#!/usr/bin/env python3
"""
Turn mood.camera recipe cards into RawTherapee partial processing profiles.

    tools/mood/mood2pp3.py [--out DIR] [--cluts DIR]

Reads recipes.json (one entry per mood.camera preset, values exactly as printed on
the "Get settings" card) and writes one .pp3 per recipe plus one per base
emulation. Profiles are *partial*: they only touch the tools the recipe needs
(Film Simulation, Film Grain, Exposure, Shadows/Highlights, Lab chromaticity,
Colour Toning Lab shift for temp/tint, Soft Light for bloom), so applying one
keeps your crop, lens correction, etc.

Mapping (mood -> RawTherapee)
  Emulation            -> Film Simulation HaldCLUT (see EMULATION_CLUT below)
  Strength LOW/MED/HIGH-> Film Simulation strength 35 / 65 / 100
  Saturation / Mute    -> Lab chromaticity  +12 / -12 per step
  Temp / Tint          -> Colour Toning "Lab regions" a/b offset (works after the CLUT)
  Grain level / size   -> Film Grain strength (14 per level) / ISO (size)
  Bloom                -> Soft Light 15 / 30 / 45 / 60
  Halation             -> Halation tool strength 20 / 35 / 50 / 70 (red-orange, radius 40)
  Aberration           -> no RawTherapee equivalent (noted in the profile header)
  Brightness           -> Exposure compensation (x1.5 EV)
  Contrast             -> Exposure contrast (x10)
  Dynamic range        -> Shadows/Highlights  MED: 20/10, HIGH: 40/25
  Curve s / m / h      -> parametric tone curve shadows / darks+lights / highlights (x12)
  Fade                 -> Exposure black level lifted (-1500 per step)
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_OUT = os.path.expanduser(
    "~/Library/Containers/com.rawtherapee.rawtherapee5/Data/Library/Application Support/RawTherapee5.13-custom/config/profiles/mood")
DEFAULT_CLUTS = os.path.expanduser("~/Downloads/HaldCLUT")

# mood.camera base emulation -> (HaldCLUT relative path, extra temp, extra tint, description)
EMULATION_CLUT = {
    "PORTRA":   ("Color/Kodak/Kodak Portra 400 2.png",             0,  0, "Soft pastel colours with warm skin tones"),
    "CHROME":   ("Color/Kodak/Kodak Ektachrome 100 VS.png",        0,  0, "Vibrant and punchy colours with a purple tint"),
    "GOLD":     ("Color/Kodak/Kodak Elite Color 200.png",          0,  0, "Warm, golden hues ideal for bright, sunny days"),
    "CINE":     ("Color/CreativePack-1/TealOrange1.png",           0,  0, "Intense orange-teal contrast, for a cinematic look"),
    "ANALOG":   ("Color/Fuji/Fuji Superia 400 2.png",              0,  0, "A retro look with saturated green tinted tones"),
    "STOCK":    ("Color/Fuji/Fuji Provia 100F.png",                0,  0, "Neutral stock"),
    "NORD":     ("Color/CreativePack-1/CrispWinter.png",           0,  0, "Cool, muted tones ideal for urban or rocky landscapes"),
    "XENON":    ("Color/Fuji/Fuji Provia 400X.png",               -1,  0, "High contrast cool tones ideal for night photography"),
    "APOLLO":   ("Color/CreativePack-1/LateSunset.png",            0,  0, "Intense pinks and reds for stunning sunsets and sunrises"),
    "ARIZONA":  ("Color/CreativePack-1/SoftWarming.png",           0,  0, "A low contrast look with warm saturated colours"),
    "CALYPSO":  ("Color/Kodak/Kodak Ektar 100.png",                0,  0, "Bright summery colours, with intense blues and oranges"),
    "TAIGA":    ("Color/Fuji/Fuji 400H 2.png",                     0,  0, "Muted, natural tones with a bluish green tint"),
    "VISTA":    ("Color/Fuji/Fuji Velvia 100 Generic.png",         0,  0, "Vibrant greens and blues enhancing natural landscapes"),
    "XPRO":     ("Color/Fuji/Fuji Superia 200 XPRO.png",           0,  0, "Cross-processed slide film"),
    "TUNGSTEN": ("Color/Fuji/Fuji Superia 800 2.png",             -2,  0, "Tungsten-balanced film (800T)"),
    "MONO":     ("Black-and-White/Ilford/Ilford HP5 Plus 400.png", 0,  0, "Balanced monochrome for classic black and white photography"),
    "NOIR":     ("Black-and-White/Kodak/Kodak TRI-X 400 4 +.png",  0,  0, "Dramatic monochrome, sensitive to cooler tones"),
}

# monochrome "tone" -> Lab a / b offsets applied after the B&W CLUT
MONO_TONE = {
    "MONO":   (0.0, 0.0),
    "SEPIA":  (0.10, 0.32),
    "HALIDE": (0.0, -0.06),
    "VENUS":  (0.28, -0.04),
}

STRENGTH = {"LOW": 35, "MEDIUM": 65, "HIGH": 100}
GRAIN_ISO = {"FINE": 120, "SMALL": 120, "MEDIUM": 800, "LARGE": 2000, "COARSE": 3600}
BLOOM = {"OFF": 0, "LOW": 15, "MEDIUM": 30, "HIGH": 45, "MAX": 60}
HALATION = {"OFF": 0, "LOW": 20, "MEDIUM": 35, "HIGH": 50, "MAX": 70}
DYNAMIC_RANGE = {"LOW": (0, 0), "MED": (20, 10), "HIGH": (40, 25)}

TEMP_A, TEMP_B = 0.018, 0.050   # Lab a/b per temp step (warm = +b, a little +a)
TINT_A = 0.03                   # Lab a per tint step (+ = magenta)
CHROMA_PER_STEP = 12
CONTRAST_PER_STEP = 10
BRIGHTNESS_EV = 1.5
CURVE_PER_STEP = 12
FADE_BLACK = -1500
GRAIN_PER_LEVEL = 14


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


def pp3(recipe, cluts):
    emu = recipe["emulation"]
    clut, extra_temp, extra_tint, _ = EMULATION_CLUT[emu]
    if not os.path.exists(os.path.join(cluts, clut)):
        print(f"warning: {recipe['slug']}: CLUT not found: {clut}", file=sys.stderr)

    temp = recipe["temp"] + extra_temp
    tint = recipe["tint"] + extra_tint
    a = TEMP_A * temp + TINT_A * tint
    b = TEMP_B * temp
    ta, tb = MONO_TONE.get(recipe.get("tone", "MONO"), (0.0, 0.0)) if emu in ("MONO", "NOIR") else (0.0, 0.0)
    a, b = clamp(a + ta, -1, 1), clamp(b + tb, -1, 1)

    chroma = clamp(CHROMA_PER_STEP * (recipe["saturation"] - recipe["mute"]), -100, 100)
    contrast = clamp(CONTRAST_PER_STEP * recipe["contrast"], -100, 100)
    comp = BRIGHTNESS_EV * recipe["brightness"]
    black = int(FADE_BLACK * recipe["fade"])
    s, m, h = recipe["curve"]
    shadows = clamp(CURVE_PER_STEP * s, -100, 100)
    darks = clamp(CURVE_PER_STEP * (s + m) / 2, -100, 100)
    lights = clamp(CURVE_PER_STEP * (m + h) / 2, -100, 100)
    highlights = clamp(CURVE_PER_STEP * h, -100, 100)
    hl, sh = DYNAMIC_RANGE[recipe["dynamic_range"]]
    grain = clamp(GRAIN_PER_LEVEL * recipe["grain_level"], 0, 100)
    bloom = BLOOM[recipe["bloom"]]
    halation = HALATION[recipe["halation"]]

    unsupported = [k for k in ("aberration",) if recipe[k] != "OFF"]
    header = (
        f"# mood.camera \"{recipe['name']}\" -> RawTherapee, generated by tools/mood/mood2pp3.py\n"
        f"# Emulation {emu} {recipe['strength']}"
        + (f" tone {recipe['tone']}" if "tone" in recipe else "")
        + f" | sat {recipe['saturation']:+g} temp {recipe['temp']:+g} tint {recipe['tint']:+g}"
        f" | grain {recipe['grain_level']} {recipe['grain_size']} halation {recipe['halation']} aberration {recipe['aberration']} bloom {recipe['bloom']}"
        f" | brightness {recipe['brightness']:+g} contrast {recipe['contrast']:+g} DR {recipe['dynamic_range']}"
        f" curve {s:+g} {m:+g} {h:+g} fade {recipe['fade']:+g} mute {recipe['mute']:+g}\n"
    )
    if unsupported:
        header += f"# not reproducible in RawTherapee: {', '.join(f'{k} {recipe[k]}' for k in unsupported)}\n"

    parts = [header, "[Version]\nAppVersion=5.13\nVersion=351\n"]
    parts.append(
        "[Exposure]\nAuto=false\n"
        f"Compensation={comp:.2f}\nContrast={contrast:.0f}\nBlack={black}\n"
        "CurveMode=Standard\n"
        f"Curve=2;0.25;0.50;0.75;{highlights:.0f};{lights:.0f};{darks:.0f};{shadows:.0f};\n"
        "Curve2=0;\n"
    )
    if hl or sh:
        parts.append(f"[Shadows & Highlights]\nEnabled=true\nHighlights={hl}\nHighlightTonalWidth=70\nShadows={sh}\nShadowTonalWidth=30\nRadius=40\nLab=false\n")
    else:
        parts.append("[Shadows & Highlights]\nEnabled=false\n")
    parts.append(f"[Luminance Curve]\nEnabled=true\nBrightness=0\nContrast=0\nChromaticity={chroma:.0f}\n")
    parts.append(f"[Film Simulation]\nEnabled=true\nClutFilename={clut}\nStrength={STRENGTH[recipe['strength']]}\n")
    parts.append(f"[FilmGrain]\nEnabled={'true' if grain > 0 else 'false'}\nIso={GRAIN_ISO[recipe['grain_size']]}\nStrength={grain:.0f}\nScale=100\nGamma=1\n")
    parts.append(f"[SoftLight]\nEnabled={'true' if bloom > 0 else 'false'}\nStrength={bloom}\n")
    parts.append(f"[Halation]\nEnabled={'true' if halation > 0 else 'false'}\nStrength={halation}\nRadius=40\nThreshold=70\nHue=15\n")
    if abs(a) > 1e-6 or abs(b) > 1e-6:
        parts.append(
            "[ColorToning]\nEnabled=true\nMethod=LabRegions\n"
            f"LabRegionA_1={a:.4f}\nLabRegionB_1={b:.4f}\nLabRegionSaturation_1=0\n"
            "LabRegionSlope_1=1\nLabRegionOffset_1=0\nLabRegionPower_1=1\n"
            "LabRegionHueMask_1=1;0.166666667;1;0.35;0.35;0.8287775246;1;0.35;0.35;\n"
            "LabRegionChromaticityMask_1=1;0.25;1;0.35;0.35;0.75;1;0.35;0.35;\n"
            "LabRegionLightnessMask_1=1;0.25;1;0.35;0.35;0.75;1;0.35;0.35;\n"
            "LabRegionMaskBlur_1=0\nLabRegionChannel_1=-1\nLabRegionsShowMask=-1\n"
        )
    else:
        parts.append("[ColorToning]\nEnabled=false\n")
    return "\n".join(parts)


def base_pp3(emu):
    clut, extra_temp, extra_tint, desc = EMULATION_CLUT[emu]
    a = clamp(TEMP_A * extra_temp + TINT_A * extra_tint, -1, 1)
    b = clamp(TEMP_B * extra_temp, -1, 1)
    out = (f"# mood.camera base emulation \"{emu.title()}\": {desc}\n"
           "[Version]\nAppVersion=5.13\nVersion=351\n\n"
           f"[Film Simulation]\nEnabled=true\nClutFilename={clut}\nStrength=100\n")
    if a or b:
        out += ("\n[ColorToning]\nEnabled=true\nMethod=LabRegions\n"
                f"LabRegionA_1={a:.4f}\nLabRegionB_1={b:.4f}\nLabRegionSaturation_1=0\n"
                "LabRegionSlope_1=1\nLabRegionOffset_1=0\nLabRegionPower_1=1\n"
                "LabRegionHueMask_1=1;0.166666667;1;0.35;0.35;0.8287775246;1;0.35;0.35;\n"
                "LabRegionChromaticityMask_1=1;0.25;1;0.35;0.35;0.75;1;0.35;0.35;\n"
                "LabRegionLightnessMask_1=1;0.25;1;0.35;0.35;0.75;1;0.35;0.35;\n"
                "LabRegionMaskBlur_1=0\nLabRegionChannel_1=-1\nLabRegionsShowMask=-1\n")
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=DEFAULT_OUT)
    ap.add_argument("--cluts", default=DEFAULT_CLUTS)
    args = ap.parse_args()

    recipes = json.load(open(os.path.join(HERE, "recipes.json")))
    os.makedirs(args.out, exist_ok=True)
    base_dir = os.path.join(args.out, "base emulations")
    os.makedirs(base_dir, exist_ok=True)

    for slug, r in sorted(recipes.items()):
        path = os.path.join(args.out, f"{r['name'].replace('/', '-')}.pp3")
        with open(path, "w") as f:
            f.write(pp3(r, args.cluts))
    for emu in sorted(EMULATION_CLUT):
        with open(os.path.join(base_dir, f"{emu.title()}.pp3"), "w") as f:
            f.write(base_pp3(emu))
    print(f"wrote {len(recipes)} recipes + {len(EMULATION_CLUT)} base emulations to {args.out}")


if __name__ == "__main__":
    main()
