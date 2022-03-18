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

#include "FileUtils.h"

#include "PS2Kbd.h"
#include "CPU.h"
#include "Mem.h"
#include "ESPectrum.h"
#include "hardpins.h"
#include "messages.h"
#include "osd.h"
#include <FS.h>
#include "Wiimote2Keys.h"
#include "sort.h"
//#include "FileUtils.h"
#include "Config.h"

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

// This is a simple driver based on the the standard SPI.h library.
// You can write a driver entirely independent of SPI.h.
// It can be optimized for your board or a different SPI port can be used.
// The driver must be derived from SdSpiBaseClass.
// See: SdFat/src/SpiDriver/SdSpiBaseClass.h
class MySpiClass : public SdSpiBaseClass {
 public:
  // Activate SPI hardware with correct speed and mode.
  void activate() {
    SPI.beginTransaction(m_spiSettings);
  }
  // Initialize the SPI bus.
  void begin(SdSpiConfig config) {
    (void)config;
    SPI.begin(14,2,12,13);
  }
  // Deactivate SPI hardware.
  void deactivate() {
    SPI.endTransaction();
  }
  // Receive a byte.
  uint8_t receive() {
    return SPI.transfer(0XFF);
  }
  // Receive multiple bytes.
  // Replace this function if your board has multiple byte receive.
  uint8_t receive(uint8_t* buf, size_t count) {
    for (size_t i = 0; i < count; i++) {
      buf[i] = SPI.transfer(0XFF);
    }
    return 0;
  }
  // Send a byte.
  void send(uint8_t data) {
    SPI.transfer(data);
  }
  // Send multiple bytes.
  // Replace this function if your board has multiple byte send.
  void send(const uint8_t* buf, size_t count) {
    for (size_t i = 0; i < count; i++) {
      SPI.transfer(buf[i]);
    }
  }
  // Save SPISettings for new max SCK frequency
  void setSckSpeed(uint32_t maxSck) {
    m_spiSettings = SPISettings(maxSck, MSBFIRST, SPI_MODE0);
  }

 private:
  SPISettings m_spiSettings;
} mySpi;

// set The Filesystem to SD
//#define THE_FS SD_ALT
//static SPIClass customSPI;
SdFs sd;
//FsFile dir;
//FsFile file;

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(20)
#define sd_cs_pin 13
#define SD_CS_PIN SDCARD_CS
#define SD_FAT_TYPE 3

 //#define USE_FAT_FILE_FLAG_CONTIGUOUS 0
 //#define ENABLE_DEDICATED_SPI 0
 //#define USE_LONG_FILE_NAMES 0
 //#define SDFAT_FILE_TYPE 3

//#define SD_CONFIG SdioConfig(FIFO_SDIO)
//#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
//#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
//#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(20), &mySpi)
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(20), &mySpi)
//#define SD_CONFIG SdSpiConfig(SD_CS_PIN)



#endif

#ifndef O_RDONLY
    #define O_RDONLY 0
#endif

#ifndef O_WRONLY
    #define O_WRONLY 1
#endif


void zx_reset();

// Globals
void IRAM_ATTR FileUtils::initFileSystem() {
#ifdef USE_INT_FLASH
// using internal storage (spi flash)
    Serial.println("Initializing internal storage...");
    if (!SPIFFS.begin())
        OSD::errorHalt(ERR_FS_INT_FAIL);

    vTaskDelay(2);

#endif
#ifdef USE_SD_CARD
// using external storage (SD card)

    Serial.println("Initializing external storage...");

    customSPI.begin(SDCARD_CLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);

    if (!SD.begin(SDCARD_CS, customSPI, SD_SPEED, "/sd")) {
        Serial.println("Card Mount Failed");
        OSD::errorHalt(ERR_FS_EXT_FAIL);
        return;
    }
    uint8_t cardType = SD.cardType();
    
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        OSD::errorHalt(ERR_FS_EXT_FAIL);
        return;
    }
    
    Serial.print("SD Card Type: ");
    if      (cardType == CARD_MMC)  Serial.println("MMC");
    else if (cardType == CARD_SD )  Serial.println("SDSC");
    else if (cardType == CARD_SDHC) Serial.println("SDHC");
    else                            Serial.println("UNKNOWN");
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    vTaskDelay(2);
#endif
#ifdef USE_SD_CARD_ALT
    int count=0;
    Serial.println("Initializing external storage...");
    while ((!sd.begin(SD_CONFIG)) && (count<20)) {
        Serial.printf("Card Mount Failed - Error Code %d\n",sd.sdErrorCode());
        count++;
        vTaskDelay(2);
    }
    return;
    



#endif
}

String FileUtils::getAllFilesFrom(const String path) {
    KB_INT_STOP;
    String listing;
#ifdef USE_SD_CARD_ALT
    FsFile root;
    root.open(path.c_str());
    FsFile file;
    file.openNext(&root,O_RDONLY);
    char tempfile [256]; 

    while (file) {
        file.openNext(&root,O_RDONLY);        
        file.getName(tempfile,255);
        String filename=tempfile;
        if (filename.startsWith(path) && !filename.startsWith(path + "/.")) {
            listing.concat(filename.substring(path.length() + 1));
            listing.concat("\n");
        }
    }
    vTaskDelay(2);

#else
    File root = THE_FS.open("/");
    File file = root.openNextFile();

    while (file) {
        file = root.openNextFile();
        String filename = file.name();
        if (filename.startsWith(path) && !filename.startsWith(path + "/.")) {
            listing.concat(filename.substring(path.length() + 1));
            listing.concat("\n");
        }
    }
    vTaskDelay(2);
#endif
    KB_INT_START;
    return listing;
}

void FileUtils::listAllFiles() {
    KB_INT_STOP;
#ifdef USE_SD_CARD_ALT
    FsFile root;
    FsFile file;
    root.open("/");
    Serial.println("fs opened");
    file.openNext(&root,O_RDONLY);        
    Serial.println("fs openednextfile");
    char tempfile [256]; 

    while (file) {        
        Serial.print("FILE: ");
        file.getName(tempfile,255);        
        Serial.println(tempfile);
        file.openNext(&root,O_RDONLY);                
        }
    

#else
    File root = THE_FS.open("/");
    Serial.println("fs opened");
    File file = root.openNextFile();
    Serial.println("fs openednextfile");

    while (file) {
        Serial.print("FILE: ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
#endif
    vTaskDelay(2);
    KB_INT_START;
}

void FileUtils::sanitizeFilename(String filename)
{
    filename.replace("\n", " ");
    filename.trim();
}

#ifdef USE_SD_CARD_ALT
FsFile FileUtils::safeOpenFileRead(String filename)
#else
File FileUtils::safeOpenFileRead(String filename)
#endif
{
    sanitizeFilename(filename);
#ifdef USE_SD_CARD_ALT
    FsFile root;
    FsFile f;
    root.open("/");    
    if (Config::slog_on)
        Serial.printf("%s '%s'\n", MSG_LOADING, filename.c_str());
    if (!root.exists(filename.c_str())) {
        KB_INT_START;
        OSD::errorHalt((String)ERR_READ_FILE + "\n" + filename);
    }
    f.open(filename.c_str(),O_RDONLY);
    

#else
    File f;
    if (Config::slog_on)
        Serial.printf("%s '%s'\n", MSG_LOADING, filename.c_str());
    if (!THE_FS.exists(filename.c_str())) {
        KB_INT_START;
        OSD::errorHalt((String)ERR_READ_FILE + "\n" + filename);
    }
    f = THE_FS.open(filename.c_str(), FILE_READ);
#endif
    vTaskDelay(2);

    return f;
}

int FileUtils::getFileEntriesFromDir(String path,char * filelist) {
    
    KB_INT_STOP;
    int count=0;
#ifdef XDEBUG
    Serial.printf("Getting entries from: '%s'\n", path.c_str());    
#endif
    //String filelist;
    //char filelist[FILE_STRING_MAX];

    
    //long bytePos=0;
    filelist[0]=0;
    if ((path!=DISK_ROM_DIR) && (path!=DISK_SNA_DIR)) strcat(filelist,"..\n");
#ifdef USE_SD_CARD_ALT
    FsFile root;
    root.open(path.c_str());
    if (!root || !root.isDir()) {
        OSD::errorHalt((String)ERR_DIR_OPEN + "\n" + root);
    }
    FsFile file;
    char tempname [256];
    file.openNext(&root,O_RDONLY);
    if (!file)
        Serial.println("No entries found!");
    while (file) {
        file.getName(tempname,255);
#ifdef XDEBUG
        Serial.printf("Found %s: %s...%ub...", (file.isDirectory() ? "DIR" : "FILE"), tempname, file.size());
#endif
        String filename = tempname;
        byte start = filename.indexOf("/", path.length()) + 1;
        byte end = filename.indexOf("/", start);
        filename = filename.substring(start, end);
#ifdef XDEBUG
        Serial.printf("%s...", filename.c_str());
#endif
        if (filename.startsWith(".")) {
#ifdef XDEBUG
            Serial.println("HIDDEN");
#endif
        } else 
        if (filename.endsWith(".txt")) {
#ifdef XDEBUG
            Serial.println("IGNORING TXT");
#endif
//        } else if (Config::arch == "48K" & file.size() > SNA_48K_SIZE) {
//            Serial.println("128K SKIP");
        } else {
            //if (filelist.indexOf(filename) < 0) {
#ifdef XDEBUG
                Serial.println("ADDING");
                Serial.printf("Free heap after adding: %d\n", ESP.getFreeHeap());
#endif
                if (strlen(filelist) + filename.length() + 1 < (FILE_STRING_MAX-strlen(MENU_SNA_TITLE+1))) {
                        count++;
                        //filelist += filename + "\n"; 
                        strcat(filelist,filename.c_str());                        
                        strcat(filelist,"\n");                        
                    } else {
#ifdef XDEBUG
                    Serial.printf ("Truncated list as string size reached %d, max capcity is %d\n",strlen(filelist),FILE_STRING_MAX);                    
#endif
                    return count;
                }
            //} else {
            //    Serial.println("EXISTS");
           // }
        }
        file.openNext(&root,O_RDONLY);
    }

#else
    File root = THE_FS.open(path.c_str());
    if (!root || !root.isDirectory()) {
        OSD::errorHalt((String)ERR_DIR_OPEN + "\n" + root);
    }
    File file = root.openNextFile();
    if (!file)
        Serial.println("No entries found!");
    while (file) {
#ifdef XDEBUG
        Serial.printf("Found %s: %s...%ub...", (file.isDirectory() ? "DIR" : "FILE"), file.name(), file.size());
#endif
        String filename = file.name();
        byte start = filename.indexOf("/", path.length()) + 1;
        byte end = filename.indexOf("/", start);
        filename = filename.substring(start, end);
#ifdef XDEBUG
        Serial.printf("%s...", filename.c_str());
#endif
        if (filename.startsWith(".")) {
#ifdef XDEBUG
            Serial.println("HIDDEN");
#endif
        } else 
        if (filename.endsWith(".txt")) {
#ifdef XDEBUG
            Serial.println("IGNORING TXT");
#endif
//        } else if (Config::arch == "48K" & file.size() > SNA_48K_SIZE) {
//            Serial.println("128K SKIP");
        } else {
   //         if (filelist.indexOf(filename) < 0) {
#ifdef XDEBUG
                Serial.println("ADDING");
                Serial.printf("Free heap after adding: %d\n", ESP.getFreeHeap());
#endif
                if (strlen(filelist) + filename.length() + 1 < (FILE_STRING_MAX-strlen(MENU_SNA_TITLE+1))) {
                        count++;
                        //filelist += filename + "\n"; 
                        strcat(filelist,filename.c_str());                        
                        strcat(filelist,"\n");                        
                    } else {
#ifdef XDEBUG
                    Serial.printf ("Truncated list as string size reached %d, max capcity is %d\n",strlen(filelist),FILE_STRING_MAX);                    
#endif
                    return count;
                }
      //      } else {
     //           Serial.println("EXISTS");
      //      }
        }
        file = root.openNextFile();
    }
#endif    
    KB_INT_START;    
    return count;
}

bool FileUtils::isDirectory(String filename)
{
    
    bool d;
#ifdef USE_SD_CARD_ALT
    FsFile file;
    file = FileUtils::safeOpenFileRead(filename);
    if (file)
    {
        KB_INT_STOP;
        d=file.isDir();
        file.close();
        KB_INT_START;
        return d;
    }
    else return false;
#else
    File file;
    file = FileUtils::safeOpenFileRead(filename);
    if (file)
    {
        KB_INT_STOP;
        d=file.isDirectory();
        file.close();
        KB_INT_START;
        return d;
    }
    else return false;
#endif
}

bool FileUtils::hasSNAextension(String filename)
{
    if (filename.endsWith(".sna")) return true;
    if (filename.endsWith(".SNA")) return true;
    return false;
}

bool FileUtils::hasZ80extension(String filename)
{
    if (filename.endsWith(".z80")) return true;
    if (filename.endsWith(".Z80")) return true;
    return false;
}


/*uint16_t FileUtils::countFileEntriesFromDir(String path) {
    static char entries[FILE_STRING_MAX];
    getFileEntriesFromDir(path,entries);
    unsigned short count = 0;
    for (unsigned short i = 0; i < strlen(entries); i++) {
        if (entries[i] == ASCII_NL) {
            count++;
        }
    }
    if (strncmp(entries,"..",2)) count--;
    return count;
}*/

uint16_t FileUtils::countFileEntriesFromDirQuick(String path) {
#ifdef XDEBUG
    Serial.printf("countFileEntriesFromDirQuick(%s)",path);
    
#endif
    unsigned short count = 0;
    KB_INT_STOP;
#ifdef USE_SD_CARD_ALT
    FsFile root;
    root.open(path.c_str());
    if (!root || !root.isDir()) {
        OSD::errorHalt((String)ERR_DIR_OPEN + "\n" + root);
    }
    FsFile file;
    file.openNext(&root,O_RDONLY);
    if (!file)
        Serial.println("No entries found!");
    while (file) {
        count++;
        file.openNext(&root,O_RDONLY);
    }
#else
    File root = THE_FS.open(path.c_str());
    if (!root || !root.isDirectory()) {
        OSD::errorHalt((String)ERR_DIR_OPEN + "\n" + root);
    }
    File file = root.openNextFile();
    if (!file)
        Serial.println("No entries found!");
    while (file) {
        count++;
        file = root.openNextFile();
    }
#endif
    KB_INT_START;
   // if (entries.substring(0,1)="..") count--;
    return count;

}

void FileUtils::loadRom(String arch, String romset) {
    KB_INT_STOP;
    String path = "/rom/" + arch + "/" + romset;
    Serial.printf("Loading ROMSET '%s'\n", path.c_str());
    byte n_roms = countFileEntriesFromDirQuick(path);
#ifdef XDEBUG
    Serial.printf("Found %d roms\n", n_roms);
#endif
    if (n_roms < 1) {
        OSD::errorHalt("No ROMs found at " + path + "\nARCH: '" + arch + "' ROMSET: " + romset);
    }
    Serial.printf("Processing %u ROMs\n", n_roms);
    for (byte f = 0; f < n_roms; f++) {
#ifdef USE_SD_CARD_ALT
        FsFile rom_f;
        char tempchar [256];
        rom_f = FileUtils::safeOpenFileRead(path + "/" + (String)f + ".rom");
        rom_f.getName(tempchar,256);
        Serial.printf("Loading ROM '%s'\n", tempchar);
#else
        File rom_f = FileUtils::safeOpenFileRead(path + "/" + (String)f + ".rom");
        Serial.printf("Loading ROM '%s'\n", rom_f.name());
#endif        
        for (int i = 0; i < rom_f.size(); i++) {
            switch (f) {
            case 0:
                Mem::rom0[i] = rom_f.read();
                break;
            case 1:
                Mem::rom1[i] = rom_f.read();
                break;

#ifdef BOARD_HAS_PSRAM

            case 2:
                Mem::rom2[i] = rom_f.read();
                break;
            case 3:
                Mem::rom3[i] = rom_f.read();
                break;
#endif
            }
        }
        rom_f.close();
    }

    KB_INT_START;
}

// Get all sna files sorted alphabetically
char * FileUtils::getSortedSnaFileList(String path)
{
    //int numfiles=countFileEntriesFromDirQuick(path);

    static char filelist[FILE_STRING_MAX];

//#ifdef XDEBUG
    //Serial.printf("Number of files in Directory: %d\n", numfiles);
    //Serial.printf("Free heap before obtaining file listing: %d\n", ESP.getFreeHeap());
//    Serial.printf("memory needed is %d x 256 = %d\n", numfiles,numfiles*256);
//#endif
    
 /*   //Check we have enough memory including leaving 8k for other stuff
    if (ESP.getFreeHeap()-8192<(numfiles * 256))
    char filecontainter [numfiles] [256];//String* filenames = (String*)malloc(numfiles * sizeof(String));

#ifdef  XDEBUG
    Serial.printf("Free heap after allocating space for file listing: %d\n", ESP.getFreeHeap());
#endif
*/
    // get string of unsorted filenames, separated by newlines
  //  String entries = 
    unsigned short count=(unsigned short)getFileEntriesFromDir(path,filelist);

/*#ifdef  XDEBUG
    Serial.printf("Free heap after obtaining file listing: %d\n", ESP.getFreeHeap());
    Serial.printf("Size of file list : %d\n", strlen(filelist));
#endif    */
    
  

    // count filenames (they always end at newline)
    /*unsigned short count = 0;
    for (unsigned short i = 0; i < entries.length(); i++) {
        if (entries.charAt(i) == ASCII_NL) {
            count++;
        }
    }*/

    
//    Serial.printf("Free heap after adding all files: %d\n", ESP.getFreeHeap());
 //   Serial.printf("Number of files retrieved: %d\n", count);
  //  Serial.printf("Attempting to allocate %d bytes\n", count * sizeof(String));

    // array of filenames
    //String* filenames = (String*)malloc(count * sizeof(String));
    // memory must be initialized to avoid crash on assign
    //memset(filenames, 0, count * sizeof(String));

    // copy filenames from string to array
    /*unsigned short ich = 0;
    unsigned short ifn = 0;
    for (unsigned short i = 0; i < entries.length(); i++) {
        if (entries.charAt(i) == ASCII_NL) {
            filenames[ifn++] = entries.substring(ich, i);
            ich = i + 1;
        }
    }*/

    // sort array
    //sortArray(filelist, count);

    // string of sorted filenames
 //   String sortedEntries = "";

    // copy filenames from array to string
    /*for (unsigned short i = 0; i < count; i++) {
        sortedEntries += filenames[i] + '\n';
    }*/

    return filelist;
}

