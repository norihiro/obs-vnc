#include <obs-module.h>
#include <obs.h>
#include <util/platform.h>
#include <util/threading.h>
#include "plugin-macros.generated.h"
#include "obs-vnc-source.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")


static const char *vncsrc_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);

	return obs_module_text("VNC source");
}

static void vncsrc_update(void *data, obs_data_t *settings);

static void *vncsrc_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	struct vnc_source *src = bzalloc(sizeof(struct vnc_source));
	src->source = source;
	obs_source_set_async_unbuffered(source, true);

	pthread_mutex_init(&src->config_mutex, NULL);

	vncsrc_update(src, settings);

	vncsrc_thread_start(src);

	return src;
}

static void vncsrc_destroy(void *data)
{
	struct vnc_source *src = data;

	vncsrc_thread_stop(src);

	vncsrc_config_destroy_member(&src->config);
	bfree(src);
}

static void vncsrc_update(void *data, obs_data_t *settings)
{
	struct vnc_source *src = data;

	pthread_mutex_lock(&src->config_mutex);

#define UPDATE_NOTIFY(src, t, n, u, v) { t x=(v); if (x!=src->config.n) { src->config.n=x; src->u=1; } }

	const char *host_name = obs_data_get_string(settings, "host_name");
	if (host_name && (!src->config.host_name || strcmp(host_name, src->config.host_name))) {
		BFREE_IF_NONNULL(src->config.host_name);
		src->config.host_name = bstrdup(host_name);
		src->need_reconnect = true;
	}

	UPDATE_NOTIFY(src, int, host_port, need_reconnect, obs_data_get_int(settings, "host_port"));

	const char *plain_passwd = obs_data_get_string(settings, "plain_passwd");
	if (plain_passwd && (!src->config.plain_passwd || strcmp(plain_passwd, src->config.plain_passwd))) {
		BFREE_IF_NONNULL(src->config.plain_passwd);
		src->config.plain_passwd = bstrdup(plain_passwd);
		src->need_reconnect = true;
	}

	UPDATE_NOTIFY(src, int, encodings, encoding_updated, obs_data_get_int(settings, "encodings"));
	UPDATE_NOTIFY(src, int, compress, encoding_updated, obs_data_get_int(settings, "compress"));
	UPDATE_NOTIFY(src, bool, jpeg, encoding_updated, obs_data_get_bool(settings, "jpeg"));
	UPDATE_NOTIFY(src, int, quality, encoding_updated, obs_data_get_int(settings, "quality"));
	UPDATE_NOTIFY(src, int, qosdscp, dscp_updated, obs_data_get_int(settings, "qosdscp"));

	src->config.skip_update_l = obs_data_get_int(settings, "skip_update_l");
	src->config.skip_update_r = obs_data_get_int(settings, "skip_update_r");
	src->config.skip_update_t = obs_data_get_int(settings, "skip_update_t");
	src->config.skip_update_b = obs_data_get_int(settings, "skip_update_b");

	pthread_mutex_unlock(&src->config_mutex);
}

static void vncsrc_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "host_port", 5900);

	obs_data_set_default_int(settings, "encodings", -1);
	obs_data_set_default_int(settings, "compress", 3);
	obs_data_set_default_bool(settings, "jpeg", 1);
	obs_data_set_default_int(settings, "quality", 5);
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

	prop = obs_properties_add_list(props, "encodings", "Preferred encodings", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
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

	obs_properties_add_int(props, "skip_update_l", obs_module_text("Skip update (left)"), 0, 32767, 1);
	obs_properties_add_int(props, "skip_update_r", obs_module_text("Skip update (right)"), 0, 32767, 1);
	obs_properties_add_int(props, "skip_update_t", obs_module_text("Skip update (top)"), 0, 32767, 1);
	obs_properties_add_int(props, "skip_update_b", obs_module_text("Skip update (bottom)"), 0, 32767, 1);

	return props;
}

static struct obs_source_info vncsrc_src_info = {
	.id = "obs_vnc_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_DO_NOT_DUPLICATE,
	.get_name = vncsrc_get_name,
	.create = vncsrc_create,
	.destroy = vncsrc_destroy,
	.update = vncsrc_update,
	.get_defaults = vncsrc_get_defaults,
	.get_properties = vncsrc_get_properties,
};

bool obs_module_load(void)
{
	obs_register_source(&vncsrc_src_info);

	blog(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload()
{
	blog(LOG_INFO, "plugin unloaded");
}
