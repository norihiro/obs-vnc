// A block below is copied from rfbproto.c.
#ifdef _WIN32

#ifdef __STRICT_ANSI__
#define _BSD_SOURCE
#define _POSIX_SOURCE
#define _XOPEN_SOURCE 600
#endif
#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <errno.h>

#define WIN32
#endif // _WIN32

#include <rfb/rfbclient.h>
#include <rfb/keysym.h>
#include <time.h>
#include <string.h>
#include <obs-module.h>
#include <util/platform.h>
#include <util/threading.h>
#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#endif // ! _WIN32
#include "plugin-macros.generated.h"
#include "obs-vnc-source.h"

#define WHEEL_STEP 120 // 120 units = 15 degrees x 8

#define debug(fmt, ...) (void)0
// #define debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

static char *vnc_passwd(rfbClient* client)
{
	struct vnc_source *src = rfbClientGetClientData(client, vncsrc_thread_start);

	pthread_mutex_lock(&src->config_mutex);
	char *ret = strdup(src->config.plain_passwd ? src->config.plain_passwd : "");
	pthread_mutex_unlock(&src->config_mutex);

	return ret;
}

static rfbBool vnc_malloc_fb(rfbClient* client)
{
	struct vnc_source *src = rfbClientGetClientData(client, vncsrc_thread_start);
	if (!src)
		return FALSE;

	debug("vnc_malloc_fb width=%d height=%d\n", client->width, client->height);

	src->frame.width = client->width;
	src->frame.height = client->height;

	src->frame.linesize[0] = client->width * 4;
	BFREE_IF_NONNULL(src->frame.data[0]);
	BFREE_IF_NONNULL(src->fb_vnc);
	src->frame.data[0] = bzalloc(src->frame.linesize[0] * client->height);
	switch (src->config.bpp) {
		case 8:
			client->frameBuffer = src->fb_vnc = bzalloc(client->width * client->height);
			break;
		case 16:
			client->frameBuffer = src->fb_vnc = bzalloc(client->width * client->height * 2);
			break;
		default:
			client->frameBuffer = src->frame.data[0];
	}

	return TRUE;
}

static inline void copy_8bit(uint8_t *dst, int dst_ls, const uint8_t *src, int src_ls, int x0, int y0, int x1, int y1)
{
	for (int y=y0; y<y1; y++) for (int x=x0; x<x1; x++) {
		uint32_t s = src[src_ls*y + x];
		dst[dst_ls*y + x*4 + 0] = ((s>>6)&3) * 255 / 3;
		dst[dst_ls*y + x*4 + 1] = ((s>>3)&7) * 255 / 7;
		dst[dst_ls*y + x*4 + 2] = ((s>>0)&7) * 255 / 7;
	}
}

static inline void copy_16bit(uint8_t *dst, int dst_ls, const uint16_t *src, int src_ls, int x0, int y0, int x1, int y1)
{
	for (int y=y0; y<y1; y++) for (int x=x0; x<x1; x++) {
		uint32_t s = src[src_ls*y + x];
		dst[dst_ls*y + x*4 + 0] = ((s>>10)&31) * 255 / 31;
		dst[dst_ls*y + x*4 + 1] = ((s>>5)&31) * 255 / 31;
		dst[dst_ls*y + x*4 + 2] = ((s>>0)&31) * 255 / 31;
	}
}

static void vnc_update(rfbClient* client, int x, int y, int w, int h)
{
	debug("vnc_update x=%d y=%d w=%d h=%d\n", x, y, w, h);

	struct vnc_source *src = rfbClientGetClientData(client, vncsrc_thread_start);
	if (!src)
		return;

	if (src->config.bpp==8 && src->frame.data[0] && src->fb_vnc)
		copy_8bit(src->frame.data[0], src->frame.linesize[0], src->fb_vnc, src->frame.width, x, y, x+w, y+h);
	else if (src->config.bpp==16 && src->frame.data[0] && src->fb_vnc)
		copy_16bit(src->frame.data[0], src->frame.linesize[0], src->fb_vnc, src->frame.width, x, y, x+w, y+h);

	if (x+w < src->config.skip_update_l)
		return;
	if (x > src->frame.width - src->config.skip_update_r)
		return;
	if (y+h < src->config.skip_update_t)
		return;
	if (y > src->frame.height - src->config.skip_update_b)
		return;

	if (!src->frame.timestamp)
		src->frame.timestamp = os_gettime_ns();
}

static void set_encodings_to_client(rfbClient *client, const struct vncsrc_conig *config)
{
	switch (config->encodings) {
		case ve_tight: client->appData.encodingsString = "tight copyrect"; break;
		case ve_zrle: client->appData.encodingsString = "zrle copyrect"; break;
		case ve_ultra: client->appData.encodingsString = "ultra copyrect"; break;
		case ve_hextile: client->appData.encodingsString = "hextile copyrect"; break;
		case ve_zlib: client->appData.encodingsString = "zlib copyrect"; break;
		case ve_corre: client->appData.encodingsString = "corre copyrect"; break;
		case ve_rre: client->appData.encodingsString = "rre copyrect"; break;
		case ve_raw: client->appData.encodingsString = "raw copyrect"; break;
		default: client->appData.encodingsString = "tight zrle ultra copyrect hextile zlib corre rre raw";
	}
	client->appData.compressLevel = config->compress;
	client->appData.enableJPEG = config->jpeg;
	client->appData.qualityLevel = config->quality;
}

static inline rfbClient *rfbc_start(struct vnc_source *src)
{
	rfbClient *client;
	if (src->config.bpp==8) {
		client = rfbGetClient(8, 1, 1);
	}
	else if (src->config.bpp==16) {
		client = rfbGetClient(5, 3, 2);
	}
	else {
		src->config.bpp = 32;
		client = rfbGetClient(8, 3, 4);
		client->format.redShift   = 16; client->format.redMax   = 255;
		client->format.greenShift =  8; client->format.greenMax = 255;
		client->format.blueShift  =  0; client->format.blueMax  = 255;
	}
	src->frame.format = VIDEO_FORMAT_BGRX;

	rfbClientSetClientData(client, vncsrc_thread_start, src);
	client->MallocFrameBuffer = vnc_malloc_fb;
	client->GotFrameBufferUpdate = vnc_update;
	client->GetPassword = vnc_passwd;
	client->programName = "obs-vnc-src";
	client->canHandleNewFBSize = 1;

	pthread_mutex_lock(&src->config_mutex);

	client->serverHost = strdup(src->config.host_name);
	client->serverPort = src->config.host_port;
	set_encodings_to_client(client, &src->config);
	client->QoS_DSCP = src->config.qosdscp;

	pthread_mutex_unlock(&src->config_mutex);

	blog(LOG_INFO, "rfbInitClient with serverHost=%s serverPort=%d", client->serverHost, client->serverPort);

	if (!rfbInitClient(client, NULL, NULL)) {
		// If failed, client has already been freed.
		return NULL;
	}

	return client;
}

static inline bool rfbc_poll(struct vnc_source *src, rfbClient *client)
{
	int ret = WaitForMessage(client, 1000);
	if (ret>0) {
		if (!HandleRFBServerMessage(client)) {
			blog(LOG_INFO, "HandleRFBServerMessage returns 0");
			return 1;
		}
	}
	else if (ret<0) {
		blog(LOG_INFO, "WaitForMessage returns %d", ret);
		return 1;
	}
	return 0; // success
}

struct vncsrc_keymouse_state_s
{
	int buttonMask;
	uint32_t mod;
};

static inline int vkey_native_to_rfb(int vkey, int modifiers)
{
	switch(vkey) {
#ifdef _WIN32
		// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
		case 0x08: return XK_BackSpace; // BACKSPACE key
		case 0x09: return XK_Tab; // TAB key
		case 0x0C: return XK_Clear; // CLEAR key
		case 0x0D: return XK_Return; // ENTER key
		case 0x10: return XK_Shift_L; // SHIFT key
		case 0x11: return XK_Control_L; // CTRL key
		case 0x12: return XK_Alt_L; // ALT key
		case 0x13: return XK_Pause; // PAUSE key
		case 0x14: return XK_Caps_Lock; // CAPS LOCK key
		case 0x15: return 0; // IME Kana mode
		//se 0x15: return 0; // IME Hangul mode
		case 0x16: return 0; // IME On
		case 0x17: return 0; // IME Junja mode
		case 0x18: return 0; // IME final mode
		case 0x19: return 0; // IME Hanja mode
		//se 0x19: return 0; // IME Kanji mode
		case 0x1A: return 0; // IME Off
		case 0x1B: return XK_Escape; // ESC key
		case 0x1C: return 0; // IME convert
		case 0x1D: return 0; // IME nonconvert
		case 0x1E: return 0; // IME accept
		case 0x1F: return 0; // IME mode change request
		case 0x21: return XK_Page_Up; // PAGE UP key
		case 0x22: return XK_Page_Down; // PAGE DOWN key
		case 0x23: return XK_End; // END key
		case 0x24: return XK_Home; // HOME key
		case 0x25: return XK_Left; // LEFT ARROW key
		case 0x26: return XK_Up; // UP ARROW key
		case 0x27: return XK_Right; // RIGHT ARROW key
		case 0x28: return XK_Down; // DOWN ARROW key
		case 0x29: return XK_Select; // SELECT key
		case 0x2A: return XK_Print; // PRINT key
		case 0x2B: return XK_Execute; // EXECUTE key
		//se 0x2C: return XK_3270_PrintScreen; // PRINT SCREEN key
		case 0x2D: return XK_Insert; // INS key
		case 0x2E: return XK_Delete; // DEL key
		case 0x2F: return XK_Help; // HELP key
		case 0x5B: return XK_Super_L; // Left Windows key (Natural keyboard)
		case 0x5C: return XK_Super_R; // Right Windows key (Natural keyboard)
		case 0x5D: return 0; // Applications key (Natural keyboard)
		case 0x5F: return 0; // Computer Sleep key
		case 0x60: return XK_KP_0; // Numeric keypad 0 key
		case 0x61: return XK_KP_1; // Numeric keypad 1 key
		case 0x62: return XK_KP_2; // Numeric keypad 2 key
		case 0x63: return XK_KP_3; // Numeric keypad 3 key
		case 0x64: return XK_KP_4; // Numeric keypad 4 key
		case 0x65: return XK_KP_5; // Numeric keypad 5 key
		case 0x66: return XK_KP_6; // Numeric keypad 6 key
		case 0x67: return XK_KP_7; // Numeric keypad 7 key
		case 0x68: return XK_KP_8; // Numeric keypad 8 key
		case 0x69: return XK_KP_9; // Numeric keypad 9 key
		case 0x6A: return XK_KP_Multiply; // Multiply key
		case 0x6B: return XK_KP_Add; // Add key
		case 0x6C: return 0; // Separator key
		case 0x6D: return XK_KP_Subtract; // Subtract key
		case 0x6E: return XK_KP_Decimal; // Decimal key
		case 0x6F: return XK_KP_Divide; // Divide key
		case 0x70: return XK_F1; // F1 key
		case 0x71: return XK_F2; // F2 key
		case 0x72: return XK_F3; // F3 key
		case 0x73: return XK_F4; // F4 key
		case 0x74: return XK_F5; // F5 key
		case 0x75: return XK_F6; // F6 key
		case 0x76: return XK_F7; // F7 key
		case 0x77: return XK_F8; // F8 key
		case 0x78: return XK_F9; // F9 key
		case 0x79: return XK_F10; // F10 key
		case 0x7A: return XK_F11; // F11 key
		case 0x7B: return XK_F12; // F12 key
		case 0x7C: return XK_F13; // F13 key
		case 0x7D: return XK_F14; // F14 key
		case 0x7E: return XK_F15; // F15 key
		case 0x7F: return XK_F16; // F16 key
		case 0x80: return XK_F17; // F17 key
		case 0x81: return XK_F18; // F18 key
		case 0x82: return XK_F19; // F19 key
		case 0x83: return XK_F20; // F20 key
		case 0x84: return XK_F21; // F21 key
		case 0x85: return XK_F22; // F22 key
		case 0x86: return XK_F23; // F23 key
		case 0x87: return XK_F24; // F24 key
		case 0x90: return XK_Num_Lock; // NUM LOCK key
		case 0x91: return XK_Scroll_Lock; // SCROLL LOCK key
		case 0xA0: return XK_Shift_L  ; // Left SHIFT key
		case 0xA1: return XK_Shift_R  ; // Right SHIFT key
		case 0xA2: return XK_Control_L; // Left CONTROL key
		case 0xA3: return XK_Control_R; // Right CONTROL key
		case 0xA4: return 0; // Left MENU key
		case 0xA5: return 0; // Right MENU key
		case 0xA6: return 0; // Browser Back key
		case 0xA7: return 0; // Browser Forward key
		case 0xA8: return 0; // Browser Refresh key
		case 0xA9: return 0; // Browser Stop key
		case 0xAA: return 0; // Browser Search key
		case 0xAB: return 0; // Browser Favorites key
		case 0xAC: return 0; // Browser Start and Home key
		case 0xAD: return 0x1008FF12; // Volume Mute key
		case 0xAE: return 0x1008FF11; // Volume Down key
		case 0xAF: return 0x1008FF13; // Volume Up key
		case 0xB0: return 0; // Next Track key
		case 0xB1: return 0; // Previous Track key
		case 0xB2: return 0; // Stop Media key
		case 0xB3: return 0; // Play/Pause Media key
		case 0xB4: return 0; // Start Mail key
		case 0xB5: return 0; // Select Media key
		case 0xB6: return 0; // Start Application 1 key
		case 0xB7: return 0; // Start Application 2 key
		case 0xFA: return 0; // Play key
		case 0xFB: return 0; // Zoom key
		case 0xFC: return 0; // Reserved
		case 0xFD: return 0; // PA1 key

#elif defined(__APPLE__) // not _WIN32
		// from SDL2-2.0.5 src/events/scancodes_darwin.h
		case 0x24: return XK_Return;
		case 0x30: return XK_Tab;
		case 0x33: return XK_Delete; // SDL_SCANCODE_BACKSPACE;
		case 0x34: return XK_Return; // SDL_SCANCODE_KP_ENTER;
		case 0x35: return XK_Escape;
		case 0x36: return XK_Super_R;
		case 0x37: return XK_Super_L;
		case 0x38: return XK_Shift_L;
		case 0x39: return XK_Caps_Lock;
		case 0x3A: return XK_Alt_L;
		case 0x3B: return XK_Control_L;
		case 0x3C: return XK_Shift_R;
		case 0x3D: return XK_Alt_R;
		case 0x3E: return XK_Control_R;
		//se 0x3F: return SDL_SCANCODE_RGUI; // Fn
		case 0x40: return XK_F17;
		case 0x41: return XK_KP_Decimal;
		case 0x43: return XK_KP_Multiply;
		case 0x45: return XK_KP_Add;
		//se 0x47: return SDL_SCANCODE_NUMLOCKCLEAR;
		case 0x48: return 0x1008FF13; // XF86XK_AudioRaiseVolume;
		case 0x49: return 0x1008FF11; // XF86XK_AudioLowerVolume;
		case 0x4A: return 0x1008FF12; // XF86XK_AudioMute;
		case 0x4B: return XK_KP_Divide;
		case 0x4C: return XK_KP_Enter;
		case 0x4E: return XK_KP_Subtract;
		case 0x4F: return XK_F18;
		case 0x50: return XK_F19;
		case 0x51: return XK_KP_Equal;
		case 0x52: return XK_KP_0;
		case 0x53: return XK_KP_1;
		case 0x54: return XK_KP_2;
		case 0x55: return XK_KP_3;
		case 0x56: return XK_KP_4;
		case 0x57: return XK_KP_5;
		case 0x58: return XK_KP_6;
		case 0x59: return XK_KP_7;
		case 0x5B: return XK_KP_8;
		case 0x5C: return XK_KP_9;
		//se 0x5D: return SDL_SCANCODE_INTERNATIONAL3;
		//se 0x5E: return SDL_SCANCODE_INTERNATIONAL1;
		//se 0x5F: return SDL_SCANCODE_KP_COMMA;
		case 0x60: return XK_F5;
		case 0x61: return XK_F6;
		case 0x62: return XK_F7;
		case 0x63: return XK_F3;
		case 0x64: return XK_F8;
		case 0x65: return XK_F9;
		//se 0x66: return SDL_SCANCODE_LANG2;
		case 0x67: return XK_F11;
		//se 0x68: return SDL_SCANCODE_LANG1;
		//se 0x69: return SDL_SCANCODE_PRINTSCREEN;
		case 0x6A: return XK_F16;
		case 0x6B: return XK_Scroll_Lock;
		case 0x6D: return XK_F10;
		//se 0x6E: return SDL_SCANCODE_APPLICATION;
		case 0x6F: return XK_F12;
		case 0x71: return XK_Pause;
		case 0x72: return XK_Insert;
		case 0x73: return XK_Home;
		case 0x74: return XK_Page_Up;
		case 0x75: return XK_Delete;
		case 0x76: return XK_F4;
		case 0x77: return XK_End;
		case 0x78: return XK_F2;
		case 0x79: return XK_Page_Down;
		case 0x7A: return XK_F1;
		case 0x7B: return XK_Left;
		case 0x7C: return XK_Right;
		case 0x7D: return XK_Down;
		case 0x7E: return XK_Up;
		//se 0x7F: return SDL_SCANCODE_POWER;

#else // Linux
		default: return vkey;
#endif
	}

#ifdef __APPLE__
	if (modifiers & INTERACT_CONTROL_KEY) switch (vkey) {
		case 0x00: return (modifiers & INTERACT_SHIFT_KEY) ? 'A' : 'a';
		case 0x01: return (modifiers & INTERACT_SHIFT_KEY) ? 'S' : 's';
		case 0x02: return (modifiers & INTERACT_SHIFT_KEY) ? 'D' : 'd';
		case 0x03: return (modifiers & INTERACT_SHIFT_KEY) ? 'F' : 'f';
		case 0x04: return (modifiers & INTERACT_SHIFT_KEY) ? 'H' : 'h';
		case 0x05: return (modifiers & INTERACT_SHIFT_KEY) ? 'G' : 'g';
		case 0x06: return (modifiers & INTERACT_SHIFT_KEY) ? 'Z' : 'z';
		case 0x07: return (modifiers & INTERACT_SHIFT_KEY) ? 'X' : 'x';
		case 0x08: return (modifiers & INTERACT_SHIFT_KEY) ? 'C' : 'c';
		case 0x09: return (modifiers & INTERACT_SHIFT_KEY) ? 'V' : 'v';
		case 0x0B: return (modifiers & INTERACT_SHIFT_KEY) ? 'B' : 'b';
		case 0x0C: return (modifiers & INTERACT_SHIFT_KEY) ? 'Q' : 'q';
		case 0x0D: return (modifiers & INTERACT_SHIFT_KEY) ? 'W' : 'w';
		case 0x0E: return (modifiers & INTERACT_SHIFT_KEY) ? 'E' : 'e';
		case 0x0F: return (modifiers & INTERACT_SHIFT_KEY) ? 'R' : 'r';
		case 0x10: return (modifiers & INTERACT_SHIFT_KEY) ? 'Y' : 'y';
		case 0x11: return (modifiers & INTERACT_SHIFT_KEY) ? 'T' : 't';
		case 0x12: return '1';
		case 0x13: return '2';
		case 0x14: return '3';
		case 0x15: return '4';
		case 0x16: return '6';
		case 0x17: return '5';
		case 0x18: return '='; // SDL_SCANCODE_EQUALS;
		case 0x19: return '9';
		case 0x1A: return '7';
		case 0x1B: return '-'; // SDL_SCANCODE_MINUS;
		case 0x1C: return '8';
		case 0x1D: return '0';
		case 0x1E: return ']'; // SDL_SCANCODE_RIGHTBRACKET;
		case 0x1F: return (modifiers & INTERACT_SHIFT_KEY) ? 'O' : 'o';
		case 0x20: return (modifiers & INTERACT_SHIFT_KEY) ? 'U' : 'u';
		case 0x21: return '['; // SDL_SCANCODE_LEFTBRACKET;
		case 0x22: return (modifiers & INTERACT_SHIFT_KEY) ? 'I' : 'i';
		case 0x23: return (modifiers & INTERACT_SHIFT_KEY) ? 'P' : 'p';
		case 0x25: return (modifiers & INTERACT_SHIFT_KEY) ? 'L' : 'l';
		case 0x26: return (modifiers & INTERACT_SHIFT_KEY) ? 'J' : 'j';
		case 0x27: return '\''; // SDL_SCANCODE_APOSTROPHE;
		case 0x28: return (modifiers & INTERACT_SHIFT_KEY) ? 'K' : 'k';
		case 0x29: return ';'; // SDL_SCANCODE_SEMICOLON;
		case 0x2A: return '\\'; // SDL_SCANCODE_BACKSLASH;
		case 0x2B: return ','; // SDL_SCANCODE_COMMA;
		case 0x2C: return '/'; // SDL_SCANCODE_SLASH;
		case 0x2D: return (modifiers & INTERACT_SHIFT_KEY) ? 'N' : 'n';
		case 0x2E: return (modifiers & INTERACT_SHIFT_KEY) ? 'M' : 'm';
		case 0x2F: return '.'; // SDL_SCANCODE_PERIOD;
		case 0x31: return ' ';
		case 0x32: return '`'; // SDL_SCANCODE_GRAVE;
	}
#endif // __APPLE__

	return 0;
};

static inline void rfbc_interact_one(struct vnc_source *src, rfbClient *client, struct vncsrc_keymouse_state_s *state, struct vncsrc_interaction_event_s *ie)
{
	switch (ie->type) {
		case mouse_click:
		case mouse_click_up:
			{
				int m = 0;
				switch(ie->button_type) {
					case MOUSE_LEFT:   m = rfbButton1Mask; break;
					case MOUSE_MIDDLE: m = rfbButton2Mask; break;
					case MOUSE_RIGHT:  m = rfbButton3Mask; break;
				}
				if (ie->type==mouse_click)
					state->buttonMask |= m;
				else
					state->buttonMask &= ~m;
			}
		case mouse_move:
			SendPointerEvent(client, ie->mouse_x, ie->mouse_y, state->buttonMask);
			break;
		case mouse_leave:
			// TODO: SendKeyEvent if alt keys were down.
			break;
		case mouse_wheel:
			debug("y_delta=%+d x_delta=%+d\n", ie->y_delta, ie->x_delta);
			for (int i=0; i<+ie->y_delta; i+=WHEEL_STEP) {
				SendPointerEvent(client, ie->mouse_x, ie->mouse_y, state->buttonMask | rfbButton4Mask);
				SendPointerEvent(client, ie->mouse_x, ie->mouse_y, state->buttonMask);
			}
			for (int i=0; i<-ie->y_delta; i+=WHEEL_STEP) {
				SendPointerEvent(client, ie->mouse_x, ie->mouse_y, state->buttonMask | rfbButton5Mask);
				SendPointerEvent(client, ie->mouse_x, ie->mouse_y, state->buttonMask);
			}
			for (int i=0; i<+ie->x_delta; i+=WHEEL_STEP) {
				SendPointerEvent(client, ie->mouse_x, ie->mouse_y, state->buttonMask | 0b01000000);
				SendPointerEvent(client, ie->mouse_x, ie->mouse_y, state->buttonMask);
			}
			for (int i=0; i<-ie->x_delta; i+=WHEEL_STEP) {
				SendPointerEvent(client, ie->mouse_x, ie->mouse_y, state->buttonMask | 0b00100000);
				SendPointerEvent(client, ie->mouse_x, ie->mouse_y, state->buttonMask);
			}
			break;
		case key_click_up:
		case key_click:
			{
				int key = *(char*)&ie->key.text;
				debug("key=%02x mod=%0x sc=%0x vkey=%0x\n", key, ie->key.native_modifiers, ie->key.native_scancode, ie->key.native_vkey);

				int vkey = vkey_native_to_rfb(ie->key.native_vkey, ie->key.modifiers);
				debug("vkey_native_to_rfb(%x) returns %x\n", ie->key.native_vkey, vkey);
#ifdef _WIN32
				if (0x01<=key && key<0x1A && ie->key.modifiers & INTERACT_CONTROL_KEY)
					key += (ie->key.modifiers & INTERACT_SHIFT_KEY) ? 0x40 : 0x60;
				else if (0x1B<=key && key<0x1F && ie->key.modifiers & INTERACT_CONTROL_KEY)
					key += 0x40;
				else
#endif // _WIN32
				if (vkey) {
					key = vkey;
				}

#ifdef __APPLE__
				int mm[][2] = {
					{ INTERACT_CONTROL_KEY, XK_Control_L },
					{ INTERACT_SHIFT_KEY,   XK_Shift_L   },
					{ 0, 0 }
				};
				if (key) for (int i=0; mm[i][0]; i++) {
					const int m = mm[i][0];
					const int k = mm[i][1];
					if (ie->key.modifiers & m && ~state->mod & m) {
						state->mod |= m;
						SendKeyEvent(client, k, TRUE);
					}
					else if (~ie->key.modifiers & m && state->mod & m) {
						state->mod &= ~m;
						SendKeyEvent(client, k, FALSE);
					}
				}
#endif // __APPLE__

				if (ie->key.modifiers & INTERACT_IS_KEY_PAD) switch (key) {
					case '0': key = XK_KP_0; break;
					case '1': key = XK_KP_1; break;
					case '2': key = XK_KP_2; break;
					case '3': key = XK_KP_3; break;
					case '4': key = XK_KP_4; break;
					case '5': key = XK_KP_5; break;
					case '6': key = XK_KP_6; break;
					case '7': key = XK_KP_7; break;
					case '8': key = XK_KP_8; break;
					case '9': key = XK_KP_9; break;
					case '.': key = XK_KP_Decimal; break;
					case '/': key = XK_KP_Divide; break;
					case '*': key = XK_KP_Multiply; break;
					case '-': key = XK_KP_Subtract; break;
					case '+': key = XK_KP_Add; break;
					case '=': key = XK_KP_Equal; break;
				}

				if (0x00 < key) {
					debug("SendKeyEvent(key=%x, click=%d)\n", key, (int)(ie->type==key_click));
					SendKeyEvent(client, key, ie->type==key_click ? TRUE : FALSE);
				}
			}
			break;
	}
}

static inline void rfbc_interact(struct vnc_source *src, rfbClient *client, struct vncsrc_keymouse_state_s *state)
{
	bool cont;
	do {
		cont = 0;
		if (pthread_mutex_trylock(&src->interact_mutex))
			break;
		if (src->interacts.size == 0) {
			pthread_mutex_unlock(&src->interact_mutex);
			break;
		}

		struct vncsrc_interaction_event_s ie;
		circlebuf_pop_front(&src->interacts, &ie, sizeof(ie));
		if (src->interacts.size > 0)
			cont = 1;
		pthread_mutex_unlock(&src->interact_mutex);

		debug("received interaction event: type=%d\n", ie.type);

		if (client)
			rfbc_interact_one(src, client, state, &ie);

	} while (cont);
}

static void *thread_main(void *data)
{
	struct vnc_source *src = data;

#ifndef _WIN32
	setpriority(PRIO_PROCESS, 0, 19);
#endif // ! _WIN32
	os_set_thread_name("vncsrc");

	int n_wait = 0;
	int cnt_failure = 0;
	rfbClient *client = NULL;
	struct vncsrc_keymouse_state_s state;

	while (src->running) {
		if (src->need_reconnect) {
			if (client) {
				rfbClientCleanup(client);
				client = NULL;
				cnt_failure = 0;
				n_wait = 0;
			}
			src->need_reconnect = false;
		}
		else if (cnt_failure && n_wait>0) {
			os_sleep_ms(100);
			n_wait--;
			continue;
		}

		if (!client) {
			client = rfbc_start(src);
			if (!pthread_mutex_trylock(&src->interact_mutex)) {
				// discard interaction queue
				while (src->interacts.size >= sizeof(struct vncsrc_interaction_event_s))
					circlebuf_pop_front(&src->interacts, NULL, sizeof(struct vncsrc_interaction_event_s));
				pthread_mutex_unlock(&src->interact_mutex);
			}
			if (!client) {
				cnt_failure += 1;
				n_wait = cnt_failure > 10 ? 100 : cnt_failure*10;
				blog(LOG_WARNING, "rfbInitClient failed, will retry in %ds", n_wait/10);
				continue;
			}
			memset(&state, 0, sizeof(state));
			cnt_failure = 0;
			n_wait = 0;
		}
		else {
			if (src->encoding_updated) {
				blog(LOG_INFO, "updating encoding settings");
				pthread_mutex_lock(&src->config_mutex);
				set_encodings_to_client(client, &src->config);
				src->encoding_updated = false;
				pthread_mutex_unlock(&src->config_mutex);
				SetFormatAndEncodings(client);
			}
			if (src->dscp_updated) {
				SetDSCP(client->sock, client->QoS_DSCP = src->config.qosdscp);
				src->dscp_updated = false;
			}
			if (rfbc_poll(src, client)) {
				rfbClientCleanup(client);
				client = NULL;
			}
		}

		if (src->frame.timestamp && src->source) {
			debug("calling obs_source_output_video\n");
			obs_source_output_video(src->source, &src->frame);
			src->frame.timestamp = 0;
		}

		rfbc_interact(src, client, &state);
	}

	if(client)
		rfbClientCleanup(client);
	client = NULL;
	return NULL;
}

void vncsrc_thread_start(struct vnc_source *src)
{
	src->running = true;
	src->need_reconnect = false;
	pthread_create(&src->thread, NULL, thread_main, src);
}

void vncsrc_thread_stop(struct vnc_source *src)
{
#ifdef _WIN32
	if (src->running) {
		src->running = false;
		pthread_join(src->thread, NULL);
	}
#else
	src->running = false;
	if (src->thread)
		pthread_join(src->thread, NULL);
	src->thread = 0;
#endif

	BFREE_IF_NONNULL(src->frame.data[0]);
	BFREE_IF_NONNULL(src->fb_vnc);
}
