#include <rfb/rfbclient.h>
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
	src->frame.data[0] = bzalloc(src->frame.linesize[0] * client->height);
	client->frameBuffer = src->frame.data[0];

	return TRUE;
}

static void vnc_update(rfbClient* client, int x, int y, int w, int h)
{
	debug("vnc_update x=%d y=%d w=%d h=%d\n", x, y, w, h);
	UNUSED_PARAMETER(x);
	UNUSED_PARAMETER(y);
	UNUSED_PARAMETER(w);
	UNUSED_PARAMETER(h);

	struct vnc_source *src = rfbClientGetClientData(client, vncsrc_thread_start);
	if (!src || !src->source)
		return;

	if (!src->frame.timestamp)
		src->frame.timestamp = os_gettime_ns();
}

static inline rfbClient *rfbc_start(struct vnc_source *src)
{
	rfbClient *client = rfbGetClient(8, 3, 4);
	client->format.redShift   = 16; client->format.redMax   = 255;
	client->format.greenShift =  8; client->format.greenMax = 255;
	client->format.blueShift  =  0; client->format.blueMax  = 255;
	src->frame.format = VIDEO_FORMAT_BGRX;

	rfbClientSetClientData(client, vncsrc_thread_start, src);
	client->MallocFrameBuffer = vnc_malloc_fb;
	client->GotFrameBufferUpdate = vnc_update;
	client->GetPassword = vnc_passwd;
	client->programName = "obs-vnc-src";

	pthread_mutex_lock(&src->config_mutex);
	client->serverHost = strdup(src->config.host_name);
	client->serverPort = src->config.host_port;
	pthread_mutex_unlock(&src->config_mutex);

	blog(LOG_INFO, "rfbInitClient with serverHost=%s serverPort=%d\n", client->serverHost, client->serverPort);

	if (!rfbInitClient(client, NULL, NULL)) {
		// If failed, client has already been freed.
		return NULL;
	}

	return client;
}

static inline bool rfbc_poll(struct vnc_source *src, rfbClient *client)
{
	int ret = WaitForMessage(client, 33*1000);
	if (ret>0) {
		if (!HandleRFBServerMessage(client))
			return 1;
	}
	else if (ret<0) {
		return 1;
	}
	return 0; // success
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
			if (!client) {
				cnt_failure += 1;
				n_wait = cnt_failure > 10 ? 100 : cnt_failure*10;
				blog(LOG_WARNING, "rfbInitClient failed, will retry in %ds\n", n_wait/10);
				continue;
			}
			cnt_failure = 0;
			n_wait = 0;
		}
		else {
			if (rfbc_poll(src, client)) {
				rfbClientCleanup(client);
				client = NULL;
			}
		}

		if (src->frame.timestamp) {
			debug("calling obs_source_output_video\n");
			obs_source_output_video(src->source, &src->frame);
			src->frame.timestamp = 0;
		}
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
	src->running = false;
	if (src->thread)
		pthread_join(src->thread, NULL);
	src->thread = 0;

	BFREE_IF_NONNULL(src->frame.data[0]);
}
