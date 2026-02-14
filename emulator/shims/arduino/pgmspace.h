#pragma once
// PROGMEM shim - on the web, everything is in RAM so these are no-ops.

#define PROGMEM
#define PGM_P const char*
#define PSTR(s) (s)

#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#define pgm_read_word(addr) (*(const uint16_t*)(addr))
#define pgm_read_dword(addr) (*(const uint32_t*)(addr))
#define pgm_read_ptr(addr) (*(const void* const*)(addr))

#define memcpy_P memcpy
#define strcmp_P strcmp
#define strncpy_P strncpy
#define strlen_P strlen
