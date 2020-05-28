#ifndef __HAL__PRINTFAB_H__
#define __HAL__PRINTFAB_H__

#ifdef ARDUINO
#include "Arduino.h"
#define MYPRINTF(x) Serial.print(F(x))
#elif defined(PLATFORMIO_FRAMEWORK_libopencm3)
#include <stdio.h>
#define MYPRINTF(x) printf(x)
#else
#error No printf abstraction.
#endif

#endif  // __HAL__PRINTFAB_H__
