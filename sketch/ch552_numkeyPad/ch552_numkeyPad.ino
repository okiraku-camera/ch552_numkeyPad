/**
 * ch552_numkeyPad.ino main sketch of Numeric keypad program using ch552g.
 * Copyright (c) 2023 Takeshi Higasa, okiraku-camera.tokyo
 * 
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 * 
 */
#include "usbCommonDescriptors/HIDClassCommon.h"
#include "ch552_usbhid.h"

#define numlock_led  11
const uint8_t cols[] = {14, 15, 16, 17};
const uint8_t rows[] = {30, 31, 32, 33, 34};
const uint8_t row_masks[] = {1, 2, 4, 8, 0x10};

// scancode to HID Usage ID table.
// these codes are defined in HIDClassCommon.h of ch55xDuino
static const uint8_t scan_to_hid[] = {			// scan hid
	HID_KEYBOARD_SC_NUM_LOCK,									// 1 		0x53
	HID_KEYBOARD_SC_KEYPAD_SLASH,							// 2		0x54
	HID_KEYBOARD_SC_KEYPAD_ASTERISK,					// 3		0x55
	HID_KEYBOARD_SC_KEYPAD_MINUS,							// 4		0x56
	HID_KEYBOARD_SC_KEYPAD_7_AND_HOME,				// 5		0x5f
	HID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW,		// 6		0x60
	HID_KEYBOARD_SC_KEYPAD_9_AND_PAGE_UP,			// 7		0x61
	HID_KEYBOARD_SC_KEYPAD_PLUS,							// 8		0x57
	HID_KEYBOARD_SC_KEYPAD_4_AND_LEFT_ARROW,	// 9		0x5c
	HID_KEYBOARD_SC_KEYPAD_5,									// 10		0x5d
	HID_KEYBOARD_SC_KEYPAD_6_AND_RIGHT_ARROW,	// 11		0x5e
	0,																				// 12
	HID_KEYBOARD_SC_KEYPAD_1_AND_END,					// 13		0x59
	HID_KEYBOARD_SC_KEYPAD_2_AND_DOWN_ARROW,	// 14		0x5a
	HID_KEYBOARD_SC_KEYPAD_3_AND_PAGE_DOWN,		// 15		0x5b
	HID_KEYBOARD_SC_KEYPAD_ENTER,							// 16		0x58
	HID_KEYBOARD_SC_KEYPAD_0_AND_INSERT,			// 17		0x62
	0,																				// 18
	HID_KEYBOARD_SC_KEYPAD_DOT_AND_DELETE,		// 19		0x63
	0																					// 20
};

// called from USBhandler
void USBStartSuspend() {
	P1 &= 0xfd;	
}

// Clarity is important
void setup() {
	usbhid_init();
	pinMode(numlock_led, OUTPUT);
	digitalWrite(numlock_led, 0);
	for(uint8_t i = 0; i < sizeof(cols); i++)
		pinMode(cols[i], INPUT_PULLUP);
	for(uint8_t i = 0; i < sizeof(rows); i++) {
		pinMode(rows[i], OUTPUT);
		digitalWrite(rows[i], 1);
	}
}

void key_event(uint8_t c, uint8_t state){
	if (c < 1 || c > sizeof(scan_to_hid) )
		return;
	uint8_t hidcode = scan_to_hid[c - 1];

	if (state) 
		report_press(hidcode);
	else
		report_release(hidcode);
}

#define scan_bytes 3
static uint8_t last_stable[scan_bytes];
static uint8_t last_scan[scan_bytes];
void scan() {
	__data uint8_t keys[scan_bytes];
	__data uint8_t key = 0;
	__data uint8_t n = 0xff;
	for(uint8_t row = 0; row < sizeof(rows); row++) {
		if (row & 1) {	// odd number row
			P3 &= ~row_masks[row];
			n |= (P1 & 0xf0);
			P3 |= row_masks[row];
			keys[key++] = ~n;
		} else {				// even number row
			P3 &= ~row_masks[row];
			n = (P1 & 0xf0) >> 4;
			P3 |= row_masks[row];
		}
	}
	n |= 0xf0;
	keys[key] = ~n;	// row = 5.
	
	n = 0;;
	for(uint8_t i = 0; i < sizeof(keys); i++) {
		// check if current and prev scan results are match.
		if (last_scan[i] != keys[i])
			n++;
		last_scan[i] = keys[i];		// save current result as prev result.
	}
	if (n)
		return;	// no match. may be de-bounced.

	n = 0;	
	for(key = 0; key < sizeof(keys); key++, n += 8) {
		uint8_t changes = keys[key] ^ last_stable[key];
		last_stable[key] = keys[key];	// save last-stable state.
		if (changes) {
			uint8_t mask = 1;
			for (uint8_t i = 0; i < 8; i++, mask <<=1 )
				if (changes & mask) key_event(n + i + 1, keys[key] & mask ? 1 : 0);
		}
	}
}

// NumLock led connected to P1.1 
#define NUMLOCK_LED_MASK  2

void loop() {
	static uint8_t led_counter = 0;
	scan();
	delay(10);	// scan interval.
	if (++led_counter > 10){ // check LED state every 100msec.
		led_counter = 0;
		if (get_hid_ledstate() & HID_KEYBOARD_LED_NUMLOCK)
			P1 |= NUMLOCK_LED_MASK;
		else
			P1 &= ~NUMLOCK_LED_MASK;
	}  
}
