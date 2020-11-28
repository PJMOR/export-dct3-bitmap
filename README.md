# Extract Nokia DCT3 Bitmaps

This is a simple program that takes a Dejan flash file (.fls) of a DCT3 generation Nokia phone and exports .pbm bitmaps for each bitmap graphic used in the phone's system, with the exception of game graphics, screensavers, and picture messages.

Place the flash file in the same directory as the code, rename it to `input.fls`, compile code with `gcc main.c`, and run with `./a.out`. The resulting bitmaps will be placed the `bitmaps` folder in the same directory as the flash file and code.

This code should work with all DCT3 firmwares, and has been tested on firmwares of the following DCT3 devices:

- 3610 (NAM-1)
- 2100 (NAM-2)
- 3410 (NHM-2)
- 6250 (NHM-3)
- 3310 (NHM-5)
- 3330 (NHM-6)
- 3350 (NHM-9)
- 6090 (NME-3)
- 6210 (NPE-3)
- 5510 (NPM-5)
- 5190 (NSB-1)
- 6190 (NSB-3)
- 8890 (NSB-6)
- 8290 (NSB-7)
- 5110 (NSE-1)
- 5110i (NSE-2)
- 6110 (NSE-3)
- 7110 (NSE-5)
- 8810 (NSE-6)
- 3210 (NSE-8)
- 5130 (NSK-1)
- 6130 (NSK-3)
- 6150 (NSM-1)
- 8850 (NSM-2)
- 8210 (NSM-3)
- 8250 (NSM-3D)
- 8855 (NSM-4)
- 5210 (NSM-5)

For a full list of firmwares tested, refer to [tested_firmwares.md](tested_firmwares.md)

More info can be found [on my website.](https://pjmor.com/extract-nokia-dct3-system-graphics/)
