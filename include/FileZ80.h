///////////////////////////////////////////////////////////////////////////////
//
// ZX-ESPectrum - ZX Spectrum emulator for ESP32
//
// Copyright (c) 2020, 2021 David Crespo [dcrespo3d]
// https://github.com/dcrespo3d/ZX-ESPectrum-Wiimote
//
// Based on previous work by Ramón Martinez, Jorge Fuertes and many others
// https://github.com/rampa069/ZX-ESPectrum
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#ifndef FileZ80_h
#define FileZ80_h

#include <Arduino.h>
#include <FS.h>

#ifdef USE_SD_CARD_ALT
// using external storage (SD card with Arduino SFAT Lib)
#include <SPI.h>
#include "SdFat.h"
#endif

class FileZ80
{
public:
    static bool IRAM_ATTR load(String z80_fn);
    // static bool IRAM_ATTR save(String z80_fn);

private:
#ifdef USE_SD_CARD_ALT
    static void loadCompressedMemData(FsFile &f, uint16_t dataLen, uint16_t memStart, uint16_t memlen);
    static void loadCompressedMemPage(FsFile &f, uint16_t dataLen, uint8_t* memPage, uint16_t memlen);
#else
    static void loadCompressedMemData(File f, uint16_t dataLen, uint16_t memStart, uint16_t memlen);
    static void loadCompressedMemPage(File f, uint16_t dataLen, uint8_t* memPage, uint16_t memlen);
#endif
};

#endif