/*
 * Lecture 3: polling loop -> event-driven workqueue.
 *
 * Task 1: the starter's polling_thread (10 ms k_sleep loop) logged
 *   events=10 total_wakeups=92 wasted=82 (89%), i.e. ~9 wasted wake-ups per event.
 * Task 2/3: sensor_sim submits sensor_work; the handler runs once per event,
 *   ~100 ms apart, and nothing wakes up in between.
 * Bonus: bursts of 5 events within 20 ms; k_work_reschedule() with a 30 ms delay
 *   collapses each burst into a single handler run.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(workqueue_demo, LOG_LEVEL_INF);

#define STACK_SIZE   1024
#define SENSOR_MS    100 /* one event every 100 ms */
#define EVENT_COUNT  10
#define BURST_COUNT  3
#define BURST_EVENTS 5
#define BURST_GAP_MS 3   /* 5 events well inside 20 ms */
#define DEBOUNCE_MS  30

static int total_events;
static int total_processed;
static uint32_t last_event_tick;
static uint32_t last_run_tick;

static void sensor_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	uint32_t now = k_uptime_get_32();

	total_processed++;
	LOG_INF("[HANDLER] processed event %d  tick=%u  since_last_run=%u ms  since_last_event=%u ms",
		total_processed, now, now - last_run_tick, now - last_event_tick);
	last_run_tick = now;
}

K_WORK_DEFINE(sensor_work, sensor_handler);
K_WORK_DELAYABLE_DEFINE(debounce_work, sensor_handler);

static void sensor_sim_fn(void *p1, void *p2, void *p3)
{
	/* Tasks 2 and 3: one event per SENSOR_MS, each handed to the system workqueue */
	for (int i = 0; i < EVENT_COUNT; i++) {
		k_msleep(SENSOR_MS);

		total_events++;
		last_event_tick = k_uptime_get_32();
		LOG_INF("[SENSOR] event %d  tick=%u", i, last_event_tick);

		int ret = k_work_submit(&sensor_work);
		if (ret < 0) {
			LOG_ERR("submit failed: %d", ret);
		}
	}

	k_msleep(SENSOR_MS);
	LOG_INF("[SUMMARY] workqueue: events=%d  handler_runs=%d", total_events, total_processed);

	/* Bonus: each burst keeps pushing the deadline out, so the handler runs once per burst */
	total_events = 0;
	total_processed = 0;

	for (int b = 0; b < BURST_COUNT; b++) {
		k_msleep(SENSOR_MS);

		for (int i = 0; i < BURST_EVENTS; i++) {
			if (i > 0) {
				k_msleep(BURST_GAP_MS);
			}

			total_events++;
			last_event_tick = k_uptime_get_32();

			int ret = k_work_reschedule(&debounce_work, K_MSEC(DEBOUNCE_MS));
			if (ret < 0) {
				LOG_ERR("reschedule failed: %d", ret);
			}
			LOG_INF("[SENSOR] burst %d event %d  reschedule tick=%u", b, i, last_event_tick);
		}
	}

	k_msleep(SENSOR_MS);
	LOG_INF("[SUMMARY] debounce: events=%d  handler_runs=%d", total_events, total_processed);
}

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_sim_fn, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	LOG_INF("=== L3: polling to workqueue ===");
	LOG_INF("sensor fires every %d ms, handler runs from the system workqueue", SENSOR_MS);
	LOG_INF("bonus: %d bursts of %d events, %d ms debounce", BURST_COUNT, BURST_EVENTS,
		DEBOUNCE_MS);

	return 0;
}
