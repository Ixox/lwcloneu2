// Source: https://gist.github.com/adnbr/2439125#file-counting-millis-c

#ifndef GET_MILLIS_h
#define GET_MILLIS_h

#include <stdint.h>

// the CTC match value in OCR1A
#define CTC_MATCH_OVERFLOW ((F_CPU/1000)/8)

void millis_init(void);
unsigned long millis(void);
void millis_setLedTimer(uint16_t size, uint16_t onAt);


#endif
