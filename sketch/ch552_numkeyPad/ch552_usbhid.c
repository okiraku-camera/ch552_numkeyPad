/**
 * ch552_usbhid.c hid interfaceusb core handler definitions of ch552_numkeyPad.
 * based on CH55xDuino USBHIDKeyboard.c 
 */
#include <stdint.h>
#include <stdbool.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"
#include "USBconstant.h"
#include "USBhandler.h"

// Flag of whether upload pointer is busy
volatile __xdata uint8_t UpPoint1_Busy = 0;

#ifdef WITH_MODIFIERS
	#define report_size	8
	#define report_start_index	2
#else
	#define report_size	6
	#define report_start_index	0
#endif
uint8_t hid_report[report_size];

void delayMicroseconds(uint16_t us);

void usbhid_init() {
	USBDeviceCfg();         // Device mode configuration
	USBDeviceEndPointCfg(); // Endpoint configuration
	USBDeviceIntCfg();      // Interrupt configuration
	UEP0_T_LEN = 0;
	UEP1_T_LEN = 0; // Pre-use send length must be cleared
	UEP2_T_LEN = 0;
//
	for(uint8_t i = 0; i < sizeof(hid_report); i++)
		hid_report[i] = 0;
}

void USB_EP1_IN() {
	UEP1_T_LEN = 0;
	UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; // Default NAK
	UpPoint1_Busy = 0;                                       // Clear busy flag
}

void USB_EP1_OUT() {
	if (U_TOG_OK) { // Discard unsynchronized packets
	}
}

uint8_t USB_EP1_send() {
	if (UsbConfig == 0)
		return 0;
	__data uint16_t waitWriteCount = 0;
	waitWriteCount = 0;
	while (UpPoint1_Busy) { // wait for 250ms or give up
		waitWriteCount++;
		delayMicroseconds(5);
		if (waitWriteCount >= 50000)
			return 0;
	}

	for (__data uint8_t i = 0; i < sizeof(hid_report); i++) // load data for upload
		Ep1Buffer[64 + i] = hid_report[i];

	UEP1_T_LEN = sizeof(hid_report); // data length
	UpPoint1_Busy = 1;
	UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // upload data and respond ACK

	return 1;
}

uint8_t get_hid_ledstate() {
	return Ep1Buffer[0]; // The only info we gets
}

void report_press(uint8_t k) {
	if (k > 0) {
		bool exist = false;
		int8_t slot = -1;
		for(uint8_t i = report_start_index; i < sizeof(hid_report); i++) {
			if (hid_report[i] == k)
					exist = true;
			if (hid_report[i] == 0 && slot == -1)
				slot = (int8_t)i;
		}
		if (slot == -1)
			return;	// full.
		if (!exist)
			hid_report[slot] = k;
	}		
	USB_EP1_send();
}

void report_release(uint8_t key) {
	if (key != 0) {
		for(uint8_t i = report_start_index; i < sizeof(hid_report); i++) {
			if (hid_report[i] == key) {
				hid_report[i] = 0;
				break;
			}
		}
	}
	USB_EP1_send();
}

void releaseAll(void) {
	for(uint8_t i = 0; i < sizeof(hid_report); i++)
		hid_report[i] = 0;
	USB_EP1_send();
}
