/**
 * ch552_usbhid.c hid interfaceusb core handler definitions of ch552_numkeyPad.
 * based on CH55xDuino USBHIDKeyboard.h
 */

#include <stdint.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

void usbhid_init(void);
void report_press(uint8_t k);
void report_release(uint8_t k);
void releaseAll(void);
uint8_t get_hid_ledstate();

#ifdef __cplusplus
} // extern "C"
#endif
