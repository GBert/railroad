// Select none or only one of the below defines USE_RULES or USE_SCRIPT
#ifdef USE_RULES 
#undef USE_RULES
#endif

#ifndef USE_SCRIPT
#define USE_SCRIPT
#endif

//define file system and SD card support
#ifndef USE_UFILESYS
#define USE_UFILESYS
#endif
#ifndef USE_SPI
#define USE_SPI
#endif
#ifndef USE_SDCARD
#define USE_SDCARD
#endif

#ifndef SDCARD_DIR
#define SDCARD_DIR
#endif

#ifndef USE_SCRIPT_FATFS_EXT
#define USE_SCRIPT_FATFS_EXT
#endif

#ifndef USE_SCRIPT_FATFS
#define USE_SCRIPT_FATFS
#endif

//Further down I define SPI

