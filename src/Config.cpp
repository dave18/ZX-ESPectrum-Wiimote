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

#include "Config.h"
#include <FS.h>
#include "PS2Kbd.h"
#include "FileUtils.h"
#include "messages.h"

#ifdef USE_INT_FLASH
// using internal storage (spi flash)
#include <SPIFFS.h>
// set The Filesystem to SPIFFS
#define THE_FS SPIFFS
#endif

#ifdef USE_SD_CARD
// using external storage (SD card)
#include <SD.h>
// set The Filesystem to SD
#define THE_FS SD
static SPIClass customSPI;
#endif

#ifdef USE_SD_CARD_ALT
// using external storage (SD card with Arduino SFAT Lib)
#include <SPI.h>
#include "SdFat.h"
#endif

#ifndef O_RDONLY
    #define O_RDONLY 0
#endif

#ifndef O_WRONLY
    #define O_WRONLY 1
#endif

String   Config::arch = "128K";
String   Config::ram_file = NO_RAM_FILE;
String   Config::romSet = "SINCLAIR";
char   Config::sna_file_list[]; // list of file names
char   Config::sna_name_list[]; // list of names (without ext, '_' -> ' ')
bool     Config::slog_on = true;

// Read config from FS
void Config::load() {
    KB_INT_STOP;
    String line;
    

    // Boot config file
    Serial.printf("Loading config file '%s':\n", DISK_BOOT_FILENAME);
#ifdef USE_SD_CARD_ALT
    FsFile f;
    f = FileUtils::safeOpenFileRead(DISK_BOOT_FILENAME);
#else    
    File f;
    f = FileUtils::safeOpenFileRead(DISK_BOOT_FILENAME);
#endif
    if (f) {
#ifdef XDEBUG
        Serial.printf("Config File is size %d\n",f.size());
#endif
    for (int i = 0; i < f.size(); i++) {
        char c = (char)f.read();
#ifdef XDEBUG
        Serial.printf("%c",c);
#endif
        if (c == '\n') {
            if (line.compareTo("slog:false") == 0) {
                slog_on = false;
                Serial.printf("  + slog:%s\n", (slog_on ? "true" : "false"));
                if (Serial)
                    Serial.end();
            } else if (line.startsWith("ram:")) {
                ram_file = line.substring(line.lastIndexOf(':') + 1);
                Serial.printf("  + ram: '%s'\n", ram_file.c_str());
            } else if (line.startsWith("arch:")) {
                arch = line.substring(line.lastIndexOf(':') + 1);
                Serial.printf("  + arch: '%s'\n", arch.c_str());
            } else if (line.startsWith("romset:")) {
                romSet = line.substring(line.lastIndexOf(':') + 1);
                Serial.printf("  + romset: '%s'\n", romSet.c_str());
            } else if (line.startsWith("slog:")) {
                slog_on = (line.substring(line.lastIndexOf(':') + 1) == "true");
                Serial.printf("  + slog_on: '%s'\n", (slog_on ? "true" : "false"));
            }
            line = "";
        } else {
            line.concat(c);
        }
    }
    f.close();
    Serial.println("Config file loaded OK");
    } else {
        Serial.println("Failed to load Config file");
    }
    KB_INT_START;
}

/*int myReplace(char * str1,char * str2,char * str3)
{
    char * p;
    p=strpbrk(str1,str2);
    while (p)
    {
        memcpy(p,srt2)
    }
}*/

void Config::loadSnapshotLists()
{
#ifdef XDEBUG
    Serial.println("loadSnapshoList()");
#endif
    KB_INT_STOP;
    strcpy(sna_file_list,MENU_SNA_TITLE);
    strcat(sna_file_list,"\n");
    strcat(sna_file_list,FileUtils::getSortedSnaFileList(DISK_SNA_DIR));
    Serial.println(sna_file_list);
    strcpy(sna_name_list,sna_file_list);
    //sna_name_list.replace(".SNA", "");
    //sna_name_list.replace(".sna", "");
    //sna_name_list.replace(".Z80", "");
    //sna_name_list.replace(".z80", "");
    //sna_name_list.replace("_", " ");
    //sna_name_list.replace("-", " ");
    KB_INT_START;
}

// Dump actual config to FS
void Config::save() {
    KB_INT_STOP;
    Serial.printf("Saving config file '%s':\n", DISK_BOOT_FILENAME);
#ifdef USE_SD_CARD_ALT
    FsFile f;
    f.open(DISK_BOOT_FILENAME, O_WRONLY | O_CREAT);
#else
    File f = THE_FS.open(DISK_BOOT_FILENAME, FILE_WRITE);
#endif
    if (f) 
    {
    // Architecture
    Serial.printf("  + arch:%s\n", arch.c_str());
    f.printf("arch:%s\n", arch.c_str());
    // ROM set
    Serial.printf("  + romset:%s\n", romSet.c_str());
    f.printf("romset:%s\n", romSet.c_str());
    // RAM SNA
    Serial.printf("  + ram:%s\n", ram_file.c_str());
    f.printf("ram:%s\n", ram_file.c_str());
    // Serial logging
    Serial.printf("  + slog:%s\n", (slog_on ? "true" : "false"));
    f.printf("slog:%s\n", (slog_on ? "true" : "false"));
    f.close();
    vTaskDelay(5);
    Serial.println("Config saved OK");
    } else Serial.println("Failed to save config file");
    KB_INT_START;

    // do not reload after saving
    // load();
    // loadSnapshotLists();
}

void Config::requestMachine(String newArch, String newRomSet, bool force)
{
    if (!force && newArch == arch) {
        Serial.printf("Config::requestMachine(newArch=%s, force=false): unchanged arch, nothing to do\n", newArch.c_str());
        return;
    }

    arch = newArch;
    romSet = newRomSet;
#ifdef XDEBUG
    Serial.printf("Requesting new machine Arch %s, Romset %s\n",arch,romSet);
#endif
    FileUtils::loadRom(arch, romSet);
}
