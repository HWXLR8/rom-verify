#!/usr/bin/env python3

import binascii
import io
from lxml import etree
import os
import zipfile

ROM_PATH = 'NES'
IGNORE = ['[ROM Hacks]',
          '[Translations]',
          '[Nintendo Vs. System]'
          ]
REGION = '(USA)'

def find_roms(dir, ignore=[]):
    roms = []
    for dirpath, dirnames, filenames in os.walk(dir):
        dirnames[:] = [d for d in dirnames if d not in ignore]
        for filename in filenames:
            roms.append(os.path.join(dirpath, filename))
    return roms


def parse_dat():
    tree = etree.parse('nes.dat')
    root = tree.getroot()
    game_crc = {}
    for game in root.findall('game'):
        name = game.get('name')
        crc = game.find('.//rom').get('crc')
        game_crc[crc] = name
    return game_crc

dat = parse_dat()
dat_size = sum(REGION in v for v in dat.values())
romset = set()

for fname in find_roms(ROM_PATH, IGNORE):
    if fname.endswith('.zip'):
        zf = open(fname, 'rb')
        zdata = zf.read()
        zf.close()
        zref = zipfile.ZipFile(io.BytesIO(zdata), 'r')
        # assume only 1 file in the archive
        unzf = zref.namelist()[0]
        # read file contents
        unz_data = zref.read(unzf)
        zref.close()
        # strip iNES header
        unz_data = unz_data[16:]
        # calculate crc
        crc = binascii.crc32(unz_data)
        crc = hex(crc)[2:].zfill(8)
    else:
        continue

    if crc in dat:
        # print(f"{unzf} matches {dat[crc]}")
        del dat[crc]
        romset.add(crc)
    else:
        print(f"{unzf} with CRC {crc} has no matches")

romset_size = len(romset)
completion_rate = round(romset_size/dat_size * 100, 2)

print(f"\nfound {romset_size} valid roms out of {dat_size}")
print(f"romset is {completion_rate}% complete")

# write missing roms to file
print("writing missing roms to MISSING")
with open('MISSING', 'w') as f:
    for k, v in dat.items():
        if REGION in v:
            f.write(f"{v}, {k}\n")
