#include "usbCommonDescriptors/HIDClassCommon.h"

//static const uint8_t max_scancode = 20;
//const uint8_t numlock_led = 11;
#define max_scancode  20
#define numlock_led  11
const uint8_t cols[] = {14, 15, 16, 17};
const uint8_t rows[] = {30, 31, 32, 33, 34};
const uint8_t row_masks[] = {1, 2, 4, 8, 0x10};

uint8_t numlock_led_state = 0;

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

// NumLock led connected to P1.1 
void numLock_Led(uint8_t on) {
	if (on)
		P1 |= 2;
	else
		P1 &= 0xfd;	
}
// このあたりは急ぐこともないので、Arduino風に記述 
void setup() {
	pinMode(numlock_led, OUTPUT);
	digitalWrite(numlock_led, 0);
	for(uint8_t i = 0; i < sizeof(cols); i++)
		pinMode(cols[i], INPUT_PULLUP);
	for(uint8_t i = 0; i < sizeof(rows); i++) {
		pinMode(rows[i], OUTPUT);
		digitalWrite(rows[i], 1);
	}
	USBSerial_print("start\n");
}

static const char hex[] = "0123456789ABCDEF";
void print_hex(uint8_t b) {
#if 0
	USBSerial_print_c(hex[(b & 0xf0) >> 4]);
	USBSerial_print_c(hex[b & 0x0f]);
#else
	USBSerial_print(hex[(b & 0xf0) >> 4]);
	USBSerial_print(hex[b & 0x0f]);
#endif
}

void key_event(uint8_t switch_num, uint8_t state) {
	if (switch_num < 1 || switch_num >= max_scancode )
		return;
	uint8_t hidcode = scan_to_hid[switch_num - 1];
	print_hex(hidcode);
#if 0
	USBSerial_print_s(" , ");
	USBSerial_print_i(state);
	USBSerial_println_only();
#else
	USBSerial_print(" , ");
	USBSerial_print(state);
	USBSerial_println();
#endif
	if (state && hidcode == HID_KEYBOARD_SC_NUM_LOCK) {
		numlock_led_state ^= 1;
		numLock_Led(numlock_led_state);
	}
}

static uint8_t last_stable[3];
static uint8_t last_scan[3];
void scan() {
	uint8_t keys[3];
	uint8_t key = 0;
	uint8_t n = 0;
#if 0
	for(uint8_t row = 0; row < sizeof(rows); row++) {
		digitalWrite(rows[row], 0);	// select a row
		n = 0;
		for (uint8_t col = 0; col < sizeof(cols); col++) // read all cols.
			n |= (digitalRead(cols[col]) << col);
		if (row & 1)
			keys[key++] |= (~(n << 4) & 0xf0); 
		else
			keys[key] = ~n & 0x0f;
		digitalWrite(rows[row], 1);	// unselect a row.
	}
#else
	for(uint8_t row = 0; row < sizeof(rows); row++) {
		P3 &= ~row_masks[row];		// select a row.
		if (row & 1)
			keys[key++] |= (~P1 & 0xf0);
		else
			keys[key] = (~P1 >> 4) & 0x0f;
		P3 |= row_masks[row];		// unselect a row.
	}

#endif
	// check if current result and prev result are match.
	n = 0;
	for(uint8_t i = 0; i < sizeof(keys); i++) {
		if (last_scan[i] != keys[i])
			n++;
		last_scan[i] = keys[i];	
	}
	if (n)
		return;	// not match. may be de-bounced.

	n = 0;	
	for(key = 0; key < sizeof(keys); key++, n += 8) {
		uint8_t changes = keys[key] ^ last_stable[key];
		last_stable[key] = keys[key];	// save previous state.
		if (changes) {
			uint8_t mask = 1;
			for (uint8_t i = 0; i < 8; i++, mask <<=1 )
				if (changes & mask) key_event(n + i + 1, keys[key] & mask ? 1 : 0);
		}
	}
}

void loop() {
	scan();
	delay(10);
}
