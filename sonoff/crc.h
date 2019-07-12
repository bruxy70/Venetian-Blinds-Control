#ifndef CRC_H
#define CRC_H

#include <EEPROM.h>

void openMemory();
void saveStatus();
void loadStatus();
void saveConfig();
void loadConfig();
void copyConfig(configuration* from,configuration* to);
void defaultConfig(configuration* c);

#endif
