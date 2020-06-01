#include <obs-module.h>
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

	const char *host_name = obs_data_get_string(settings, "host_name");
	if (host_name && (!src->config.host_name || strcmp(host_name, src->config.host_name))) {
		BFREE_IF_NONNULL(src->config.host_name);
		src->config.host_name = bstrdup(host_name);
		src->config_updated = true;
		src->need_reconnect = true;
	}

	int host_port = obs_data_get_int(settings, "host_port");
	if(host_port != src->config.host_port) {
		src->config.host_port = host_port;
		src->config_updated = true;
		src->need_reconnect = true;
	}

	const char *plain_passwd = obs_data_get_string(settings, "plain_passwd");
	if (plain_passwd && (!src->config.plain_passwd || strcmp(plain_passwd, src->config.plain_passwd))) {
		BFREE_IF_NONNULL(src->config.plain_passwd);
		src->config.plain_passwd = bstrdup(plain_passwd);
		src->config_updated = true;
		src->need_reconnect = true;
	}

	pthread_mutex_unlock(&src->config_mutex);
}

static void vncsrc_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "host_port", 5900);
}

static obs_properties_t *vncsrc_get_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	obs_properties_t *props;
	// obs_property_t *prop;
	props = obs_properties_create();

	obs_properties_add_text(props, "host_name", obs_module_text("Host name"), OBS_TEXT_DEFAULT);
	obs_properties_add_int(props, "host_port", obs_module_text("Host port"), 1, 32767, 1);
	obs_properties_add_text(props, "plain_passwd", obs_module_text("Password"), OBS_TEXT_PASSWORD);

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
