#include <obs-module.h>
#include <obs.h>
#include <util/platform.h>
#include <util/threading.h>
#include "plugin-macros.generated.h"
#include "obs-vnc-source.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define debug(fmt, ...) (void)0
// #define debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

static const char *vncsrc_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);

	return obs_module_text("VNC source");
}

static void vncsrc_update(void *data, obs_data_t *settings);

static void *vncsrc_create(obs_data_t *settings, obs_source_t *source)
{
	struct vnc_source *src = bzalloc(sizeof(struct vnc_source));
	src->source = source;
	obs_source_set_async_unbuffered(source, true);

	pthread_mutex_init(&src->config_mutex, NULL);
	deque_init(&src->interacts);
	pthread_mutex_init(&src->interact_mutex, NULL);

	vncsrc_update(src, settings);

	vncsrc_thread_start(src);

	return src;
}

static void vncsrc_destroy(void *data)
{
	struct vnc_source *src = data;

	vncsrc_thread_stop(src);

	vncsrc_config_destroy_member(&src->config);
	deque_free(&src->interacts);
	bfree(src);
}

static void vncsrc_update(void *data, obs_data_t *settings)
{
	struct vnc_source *src = data;

	pthread_mutex_lock(&src->config_mutex);

#define UPDATE_NOTIFY(src, t, n, u, v)     \
	do {                               \
		t x = (v);                 \
		if (x != src->config.n) {  \
			src->config.n = x; \
			src->u = 1;        \
		}                          \
	} while (0)

#define UPDATE_NOTIFY_RECONNECT(src, t, n, v)          \
	do {                                           \
		t x = (v);                             \
		if (x != src->config.n) {              \
			src->config.n = x;             \
			vncsrc_request_reconnect(src); \
		}                                      \
	} while (0)

	const char *host_name = obs_data_get_string(settings, "host_name");
	if (host_name && (!src->config.host_name || strcmp(host_name, src->config.host_name))) {
		BFREE_IF_NONNULL(src->config.host_name);
		src->config.host_name = bstrdup(host_name);
		vncsrc_request_reconnect(src);
	}

	UPDATE_NOTIFY_RECONNECT(src, int, host_port, (int)obs_data_get_int(settings, "host_port"));

	const char *plain_passwd = obs_data_get_string(settings, "plain_passwd");
	if (plain_passwd && (!src->config.plain_passwd || strcmp(plain_passwd, src->config.plain_passwd))) {
		BFREE_IF_NONNULL(src->config.plain_passwd);
		src->config.plain_passwd = bstrdup(plain_passwd);
		vncsrc_request_reconnect(src);
	}

	src->config.rfb_timeout = (unsigned int)obs_data_get_int(settings, "rfb_timeout");

#ifdef LIBVNCSERVER_HAVE_SASL
	const char *user_name = obs_data_get_string(settings, "user_name");
	if (user_name && !*user_name) {
		BFREE_IF_NONNULL(src->config.user_name);
		vncsrc_request_reconnect(src);
	}
	else if (user_name && (!src->config.user_name || strcmp(user_name, src->config.user_name))) {
		BFREE_IF_NONNULL(src->config.user_name);
		src->config.user_name = bstrdup(user_name);
		vncsrc_request_reconnect(src);
	}
#endif // LIBVNCSERVER_HAVE_SASL

	UPDATE_NOTIFY_RECONNECT(src, int, bpp, (int)obs_data_get_int(settings, "bpp"));
	UPDATE_NOTIFY(src, int, encodings, encoding_updated, (int)obs_data_get_int(settings, "encodings"));
	UPDATE_NOTIFY(src, int, compress, encoding_updated, (int)obs_data_get_int(settings, "compress"));
	UPDATE_NOTIFY(src, bool, jpeg, encoding_updated, obs_data_get_bool(settings, "jpeg"));
	UPDATE_NOTIFY(src, int, quality, encoding_updated, (int)obs_data_get_int(settings, "quality"));
	UPDATE_NOTIFY(src, int, qosdscp, dscp_updated, (int)obs_data_get_int(settings, "qosdscp"));
	src->config.connect_opt = (int)obs_data_get_int(settings, "connect_opt");
	src->config.clear_at_disconnect = obs_data_get_bool(settings, "clear_at_disconnect");

	UPDATE_NOTIFY(src, int, skip_update_l, skip_updated, (int)obs_data_get_int(settings, "skip_update_l"));
	UPDATE_NOTIFY(src, int, skip_update_r, skip_updated, (int)obs_data_get_int(settings, "skip_update_r"));
	UPDATE_NOTIFY(src, int, skip_update_t, skip_updated, (int)obs_data_get_int(settings, "skip_update_t"));
	UPDATE_NOTIFY(src, int, skip_update_b, skip_updated, (int)obs_data_get_int(settings, "skip_update_b"));

	pthread_mutex_unlock(&src->config_mutex);

#undef UPDATE_NOTIFY
}

static void vncsrc_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "host_port", 5900);
#ifdef _WIN32
	obs_data_set_default_int(settings, "rfb_timeout", 10);
#else
	obs_data_set_default_int(settings, "rfb_timeout", 60);
#endif

	obs_data_set_default_int(settings, "bpp", 32);
	obs_data_set_default_int(settings, "encodings", -1);
	obs_data_set_default_int(settings, "compress", 3);
	obs_data_set_default_bool(settings, "jpeg", 1);
	obs_data_set_default_int(settings, "quality", 5);
}

bool reconnect_cb(obs_properties_t *props, obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	struct vnc_source *src = data;
	vncsrc_request_reconnect(src);

	return false;
}

static obs_properties_t *vncsrc_get_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	obs_properties_t *props;
	obs_property_t *prop;
	props = obs_properties_create();

	obs_properties_add_text(props, "host_name", obs_module_text("Host name"), OBS_TEXT_DEFAULT);
	obs_properties_add_int(props, "host_port", obs_module_text("Host port"), 1, 65535, 1);
	obs_properties_add_text(props, "plain_passwd", obs_module_text("Password"), OBS_TEXT_PASSWORD);
#ifdef LIBVNCSERVER_HAVE_SASL
	obs_properties_add_text(props, "user_name", obs_module_text("User name"), OBS_TEXT_DEFAULT);
#endif // LIBVNCSERVER_HAVE_SASL
#ifdef _WIN32
	/* On non-Windows system, signals will interrupt the connection so that
	 * it is unnecessary to have this property. */
	prop = obs_properties_add_int(props, "rfb_timeout", obs_module_text("Connection timeout"), 1, 120, 1);
	obs_property_int_set_suffix(prop, " s");
#endif

	obs_properties_add_button(props, "reconnect", obs_module_text("Reconnect"), reconnect_cb);

	prop = obs_properties_add_list(props, "bpp", obs_module_text("Color level"), OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(prop, "24-bit", 32);
	obs_property_list_add_int(prop, "16-bit", 16);
	obs_property_list_add_int(prop, "8-bit", 8);
	prop = obs_properties_add_list(props, "encodings", obs_module_text("Preferred encodings"), OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(prop, "Auto", -1);
	obs_property_list_add_int(prop, "Tight", ve_tight);
	obs_property_list_add_int(prop, "ZRLE", ve_zrle);
	obs_property_list_add_int(prop, "Ultra", ve_ultra);
	obs_property_list_add_int(prop, "Hextile", ve_hextile);
	obs_property_list_add_int(prop, "ZLib", ve_zlib);
	obs_property_list_add_int(prop, "CoRRE", ve_corre);
	obs_property_list_add_int(prop, "RRE", ve_rre);
	obs_property_list_add_int(prop, "Raw", ve_raw);
	obs_properties_add_int(props, "compress", obs_module_text("Compress level"), 0, 9, 1);
	obs_properties_add_bool(props, "jpeg", obs_module_text("Enable JPEG"));
	obs_properties_add_int(props, "quality", obs_module_text("Quality level for JPEG (0=poor, 9=best)"), 0, 9, 1);
	obs_properties_add_int(props, "qosdscp", obs_module_text("QoS DSCP"), 0, 255, 1);
	prop = obs_properties_add_list(props, "connect_opt", obs_module_text("Connection timing"), OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(prop, obs_module_text("Always"), connect_always);
	obs_property_list_add_int(prop, obs_module_text("Connect at shown"), connect_at_shown);
	obs_property_list_add_int(prop, obs_module_text("Connect at shown, disconnect at hidden"),
				  connect_at_shown_disconnect_at_hidden);
	obs_property_list_add_int(prop, obs_module_text("Connect at activated"), connect_at_active);
	obs_property_list_add_int(prop, obs_module_text("Connect at activated, disconnect at hidden"),
				  connect_at_active_disconnect_at_hidden);
	obs_property_list_add_int(prop, obs_module_text("Connect at activated, disconnect at inactive"),
				  connect_at_active_disconnect_at_inactive);
	obs_properties_add_bool(props, "clear_at_disconnect", obs_module_text("Clear texture at disconnection"));

	obs_properties_add_int(props, "skip_update_l", obs_module_text("Skip update (left)"), 0, 32767, 1);
	obs_properties_add_int(props, "skip_update_r", obs_module_text("Skip update (right)"), 0, 32767, 1);
	obs_properties_add_int(props, "skip_update_t", obs_module_text("Skip update (top)"), 0, 32767, 1);
	obs_properties_add_int(props, "skip_update_b", obs_module_text("Skip update (bottom)"), 0, 32767, 1);

	return props;
}

static inline void queue_interaction(struct vnc_source *src, struct vncsrc_interaction_event_s *interact)
{
	debug("queuing interaction event: type=%d\n", interact->type);
	if (!pthread_mutex_lock(&src->interact_mutex)) {
		deque_push_back(&src->interacts, interact, sizeof(*interact));
		pthread_mutex_unlock(&src->interact_mutex);
	}
}

static void vncsrc_show(void *data)
{
	struct vnc_source *src = data;
	long f = os_atomic_load_long(&src->display_flags);
	os_atomic_set_long(&src->display_flags, f | VNCSRC_FLG_SHOWN);
}

static void vncsrc_hide(void *data)
{
	struct vnc_source *src = data;
	long f = os_atomic_load_long(&src->display_flags);
	os_atomic_set_long(&src->display_flags, f & ~VNCSRC_FLG_SHOWN);
}

static void vncsrc_activate(void *data)
{
	struct vnc_source *src = data;
	long f = os_atomic_load_long(&src->display_flags);
	os_atomic_set_long(&src->display_flags, f | VNCSRC_FLG_ACTIVE);
}

static void vncsrc_deactivate(void *data)
{
	struct vnc_source *src = data;
	long f = os_atomic_load_long(&src->display_flags);
	os_atomic_set_long(&src->display_flags, f & ~VNCSRC_FLG_ACTIVE);
}

static void vncsrc_mouse_click(void *data, const struct obs_mouse_event *event, int32_t type, bool up, uint32_t count)
{
	UNUSED_PARAMETER(count);

	struct vnc_source *src = data;
	struct vncsrc_interaction_event_s interact = {
		.type = up ? mouse_click_up : mouse_click,
		.mouse_x = event->x,
		.mouse_y = event->y,
		.button_type = type,
	};
	queue_interaction(src, &interact);
}

static void vncsrc_mouse_move(void *data, const struct obs_mouse_event *event, bool mouse_leave_)
{
	struct vnc_source *src = data;
	struct vncsrc_interaction_event_s interact = {
		.type = mouse_leave_ ? mouse_leave : mouse_move,
		.mouse_x = event->x,
		.mouse_y = event->y,
	};
	queue_interaction(src, &interact);
}

static void vncsrc_mouse_wheel(void *data, const struct obs_mouse_event *event, int x_delta, int y_delta)
{
	struct vnc_source *src = data;
	struct vncsrc_interaction_event_s interact = {
		.type = mouse_wheel,
		.mouse_x = event->x,
		.mouse_y = event->y,
		.x_delta = x_delta,
		.y_delta = y_delta,
	};
	queue_interaction(src, &interact);
}

static void vncsrc_key_click(void *data, const struct obs_key_event *event, bool key_up)
{
	struct vnc_source *src = data;
	struct vncsrc_interaction_event_s interact = {
		.type = key_up ? key_click_up : key_click,
		.modifiers = event->modifiers,
		.native_vkey = event->native_vkey,
	};
	strncpy(interact.text, event->text, sizeof(interact.text) - 1);
	queue_interaction(src, &interact);
}

static struct obs_source_info vncsrc_src_info = {
	.id = "obs_vnc_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_DO_NOT_DUPLICATE | OBS_SOURCE_INTERACTION,
	.get_name = vncsrc_get_name,
	.create = vncsrc_create,
	.destroy = vncsrc_destroy,
	.update = vncsrc_update,
	.get_defaults = vncsrc_get_defaults,
	.get_properties = vncsrc_get_properties,
	.show = vncsrc_show,
	.hide = vncsrc_hide,
	.activate = vncsrc_activate,
	.deactivate = vncsrc_deactivate,
	.mouse_click = vncsrc_mouse_click,
	.mouse_move = vncsrc_mouse_move,
	.mouse_wheel = vncsrc_mouse_wheel,
	.key_click = vncsrc_key_click,
};

bool obs_module_load(void)
{
	obs_register_source(&vncsrc_src_info);

	blog(LOG_INFO, "plugin loaded (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload()
{
	blog(LOG_INFO, "plugin unloaded");
}
