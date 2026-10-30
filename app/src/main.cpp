#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>

LOG_MODULE_REGISTER(pipeline, LOG_LEVEL_INF);

#define STACK_SIZE     1024
#define SENSOR_MS      100
#define HEALTH_MS      200
#define QUEUE_SIZE     8
#define WDT_TIMEOUT_MS 1000
#define STUCK_AT_SEQ   10
#define STUCK_MS       5000

struct sensor_data {
	int32_t temperature;
	uint32_t seq;
};

K_MSGQ_DEFINE(sensor_queue, sizeof(struct sensor_data), QUEUE_SIZE, 4);

static void wdt_cb(int channel_id, void *user_data)
{
	LOG_ERR("[WDT] channel %d expired, consumer is stuck", channel_id);
}

static void producer_fn(void *p1, void *p2, void *p3)
{
	struct sensor_data data = {0, 0};

	while (1) {
		k_msleep(SENSOR_MS);
		data.seq++;
		data.temperature = 20 + data.seq % 10;

		if (k_msgq_put(&sensor_queue, &data, K_NO_WAIT) != 0) {
			LOG_WRN("[PRODUCER] queue full, dropped seq=%u", data.seq);
		}
	}
}

static void consumer_fn(void *p1, void *p2, void *p3)
{
	struct sensor_data data;
	int wdt_id = task_wdt_add(WDT_TIMEOUT_MS, wdt_cb, NULL);

	while (1) {
		k_msgq_get(&sensor_queue, &data, K_FOREVER);
		task_wdt_feed(wdt_id);
		LOG_INF("[CONSUMER] seq=%u temp=%d", data.seq, data.temperature);

		if (data.seq == STUCK_AT_SEQ) {
			LOG_WRN("[CONSUMER] stuck for %d ms", STUCK_MS);
			k_msleep(STUCK_MS);
		}
	}
}

static void health_fn(void *p1, void *p2, void *p3)
{
	while (1) {
		k_msleep(HEALTH_MS);
		uint32_t used = k_msgq_num_used_get(&sensor_queue);

		if (used * 4 >= QUEUE_SIZE * 3) {
			LOG_WRN("[HEALTH] queue at %u/%u", used, QUEUE_SIZE);
		}
	}
}

K_THREAD_DEFINE(producer_thread, STACK_SIZE, producer_fn, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(consumer_thread, STACK_SIZE, consumer_fn, NULL, NULL, NULL, 6, 0, 0);
K_THREAD_DEFINE(health_thread, STACK_SIZE, health_fn, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	task_wdt_init(NULL);
	return 0;
}
