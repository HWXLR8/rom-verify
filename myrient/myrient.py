#!/usr/bin/env python3

import os
import subprocess
import sys

consoles = {
    "NES" : [
        "/files/No-Intro/Nintendo - Nintendo Entertainment System (Headered)/",
        "/files/No-Intro/Nintendo - Nintendo Entertainment System (Headered) (Private)/",
    ],

    "SNES" : [
        "/files/No-Intro/Nintendo - Super Nintendo Entertainment System/",
        "/files/No-Intro/Nintendo - Super Nintendo Entertainment System (Private)/",
    ],

    "GENESIS" : [
        "/files/No-Intro/Sega - Mega Drive - Genesis/",
        "/files/No-Intro/Sega - Mega Drive - Genesis (Private)/",
    ],
}

if len(sys.argv) < 2:
    print("Usage: myrient [ROM_PATH]")
    sys.exit(1)

ROM_PATH = sys.argv[1]
os.chdir(ROM_PATH)

for console, variations in consoles.items():
    print(f"creating directory {console}")
    os.makedirs(console, exist_ok=True)
    os.chdir(console)
    for v in variations:
        print(f'copying {v}')
        subprocess.Popen(f'rclone copy "myrient:{v}" . --progress --http-no-head', shell=True)
    os.chdir(ROM_PATH)
