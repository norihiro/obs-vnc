#ifndef OBS_VNC_SOURCE_H
#define OBS_VNC_SOURCE_H

#include <obs.h>
#include <util/threading.h>
#include <util/circlebuf.h>
#include <rfb/rfbconfig.h>

enum vnc_encodings_e {
	ve_tight = 1,
	ve_zrle = 2,
	ve_ultra = 4,
	ve_hextile = 8,
	ve_zlib = 16,
	ve_corre = 32,
	ve_rre = 64,
	ve_raw = 128,
};

struct vncsrc_conig
{
	char *host_name;
	int host_port;
#ifdef LIBVNCSERVER_HAVE_SASL
	char *user_name;
#endif // LIBVNCSERVER_HAVE_SASL
	char *plain_passwd;
	unsigned int rfb_timeout;
	int bpp; // bits per pixel; 8, 16, [32] only.
	int encodings;
	int compress;
	bool jpeg;
	int quality;
	int qosdscp;
	enum connect_e {
		connect_always = 0,
		connect_at_shown = 1,
		connect_at_active = 2,
		connect_at_shown_disconnect_at_hidden = 1 + 4,
		connect_at_active_disconnect_at_hidden = 2 + 4,
		connect_at_active_disconnect_at_inactive = 2 + 8,
	} connect_opt;

	int skip_update_l, skip_update_r, skip_update_t, skip_update_b;
};

struct vncsrc_interaction_event_s
{
	enum type_e {
		mouse_click,
		mouse_click_up,
		mouse_move,
		mouse_leave,
		mouse_wheel,
		key_click,
		key_click_up,
	} type;
	union {
		// key
		struct
		{
			uint32_t modifiers;
			char text[8];
			uint32_t native_vkey;
		};
		// mouse_click, mouse_wheel
		struct
		{
			int32_t mouse_x, mouse_y;
			int32_t button_type;
			int x_delta;
			int y_delta;
		};
	};
};

// for display_flags
#define VNCSRC_FLG_SHOWN 1
#define VNCSRC_FLG_ACTIVE 2

struct vnc_source
{
	pthread_mutex_t config_mutex;
	volatile struct vncsrc_conig config;
	obs_source_t *source;
	volatile bool need_reconnect;
	volatile bool encoding_updated;
	volatile bool dscp_updated;
	volatile bool skip_updated;
	volatile bool running;
	volatile long display_flags;

	struct obs_source_frame frame;
	void *fb_vnc; // for 8-bit and 16-bit

	pthread_mutex_t interact_mutex;
	struct circlebuf interacts;

	// threads
	pthread_t thread;
};

void vncsrc_thread_start(struct vnc_source *src);
void vncsrc_thread_stop(struct vnc_source *src);

#define BFREE_IF_NONNULL(x) \
	if (x) {            \
		bfree(x);   \
		(x) = NULL; \
	}

static inline void vncsrc_config_destroy_member(volatile struct vncsrc_conig *c)
{
	BFREE_IF_NONNULL(c->host_name);
	BFREE_IF_NONNULL(c->plain_passwd);
}

#endif // OBS_VNC_SOURCE_H
