#!/usr/bin/env python3

import os
import subprocess
import sys

paths = [
    # NES
    "/files/No-Intro/Nintendo - Nintendo Entertainment System (Headered)/",
    "/files/No-Intro/Nintendo - Nintendo Entertainment System (Headered) (Private)/",
    # SNES
    "/files/No-Intro/Nintendo - Super Nintendo Entertainment System/",
    "/files/No-Intro/Nintendo - Super Nintendo Entertainment System (Private)/",
    # GENESIS
    "/files/No-Intro/Sega - Mega Drive - Genesis/",
    "/files/No-Intro/Sega - Mega Drive - Genesis (Private)/",
    # GB
    "/files/No-Intro/Nintendo - Game Boy/",
    "/files/No-Intro/Nintendo - Game Boy (Private)/",
    # GBC
    "/files/No-Intro/Nintendo - Game Boy Color/",
    "/files/No-Intro/Nintendo - Game Boy Color (Private)/",
    # GBA
    "/files/No-Intro/Nintendo - Game Boy Advance/",
    "/files/No-Intro/Nintendo - Game Boy Advance (Multiboot)/",
    "/files/No-Intro/Nintendo - Game Boy Advance (Play-Yan)/",
    "/files/No-Intro/Nintendo - Game Boy Advance (Private)/",
    "/files/No-Intro/Nintendo - Game Boy Advance (Video)/",
    "/files/No-Intro/Nintendo - Game Boy Advance (e-Reader)/",
    # N64
    "/files/No-Intro/Nintendo - Nintendo 64 (BigEndian)/",
    "/files/No-Intro/Nintendo - Nintendo 64 (BigEndian) (Private)/",
    "/files/No-Intro/Nintendo - Nintendo 64DD/",
    # NDS
    "/files/No-Intro/Nintendo - Nintendo DS (DSvision SD cards)/",
    "/files/No-Intro/Nintendo - Nintendo DS (Decrypted)/",
    "/files/No-Intro/Nintendo - Nintendo DS (Decrypted) (Private)/",
    "/files/No-Intro/Nintendo - Nintendo DS (Download Play)/",
    "/files/No-Intro/Nintendo - Nintendo DSi (Decrypted)/",
    "/files/No-Intro/Nintendo - Nintendo DSi (Digital)/",
    "/files/No-Intro/Nintendo - Nintendo DSi (Digital) (CDN) (Decrypted)/",
    # misc
    "/files/No-Intro/Nintendo - Misc/",
]

if len(sys.argv) < 2:
    print("Usage: myrient [ROM_PATH]")
    sys.exit(1)

ROM_PATH = sys.argv[1]
os.chdir(ROM_PATH)

for p in paths:
    d = os.path.basename(os.path.normpath(p))
    os.makedirs(d, exist_ok=True)
    os.chdir(d)
    print(f'copying {d}')
    p = subprocess.Popen(f'rclone copy "myrient:{p}" . --progress --http-no-head --size-only', shell=True)
    p.wait()
    os.chdir(ROM_PATH)
