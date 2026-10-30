#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(zbus_demo, LOG_LEVEL_INF);

#define STACK_SIZE 1024
#define SENSOR_MS  100

struct sensor_data {
	int32_t temperature;
	uint32_t seq;
};

static void display_cb(const struct zbus_channel *chan);

ZBUS_LISTENER_DEFINE(display_lis, display_cb);
ZBUS_SUBSCRIBER_DEFINE(logger_sub, 4);

ZBUS_CHAN_DEFINE(sensor_chan, struct sensor_data, NULL, NULL,
		 ZBUS_OBSERVERS(display_lis, logger_sub), ZBUS_MSG_INIT(0));

static void display_cb(const struct zbus_channel *chan)
{
	const struct sensor_data *data = (const struct sensor_data *)zbus_chan_const_msg(chan);

	LOG_INF("[DISPLAY] seq=%u temp=%d", data->seq, data->temperature);
}

static void sensor_fn(void *p1, void *p2, void *p3)
{
	struct sensor_data data = {0, 0};

	while (1) {
		k_msleep(SENSOR_MS);
		data.seq++;
		data.temperature = 20 + data.seq % 10;

		int ret = zbus_chan_pub(&sensor_chan, &data, K_MSEC(10));
		if (ret < 0) {
			LOG_ERR("publish failed: %d", ret);
		}
	}
}

static void logger_fn(void *p1, void *p2, void *p3)
{
	const struct zbus_channel *chan;
	struct sensor_data data;

	while (zbus_sub_wait(&logger_sub, &chan, K_FOREVER) == 0) {
		zbus_chan_read(chan, &data, K_MSEC(10));
		/* simulate slow logging output */
		k_msleep(50);
		LOG_INF("[LOGGER] seq=%u temp=%d", data.seq, data.temperature);
	}
}

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_fn, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(logger_thread, STACK_SIZE, logger_fn, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	return 0;
}
