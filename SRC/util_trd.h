#pragma once
#include "inttypes.h"
#include "stdbool.h"

bool ReadCatalog(char *file_name,char *disk_name, bool open_file);
bool ReadCatalog_b(char *file_name, bool open_file);
bool ReadCatalog_scl(char *file_name, char *disk_name,  bool open_file);
void MenuTRDOS ();
//char track0 [2304];
//-------------------
bool Run_file_scl(char *file_name, bool open_file);
void disk_autorun (void);
