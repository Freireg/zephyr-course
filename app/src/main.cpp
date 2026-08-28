#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sched_demo, LOG_LEVEL_INF);

#define STACK_SIZE 1024

static void t_low_fn(void *a, void *b, void *c)
{
	while (1) {
		LOG_INF("T_LOW running");
		k_msleep(300);
	}
}

static void t_med_fn(void *a, void *b, void *c)
{
	while (1) {
		LOG_INF("T_MED running");
		k_msleep(200);
	}
}

static void t_high_fn(void *a, void *b, void *c)
{
	while (1) {
		LOG_INF("T_HIGH running");
		k_msleep(100);
	}
}

static void t_coop_fn(void *a, void *b, void *c)
{
	while (1) {
		for (int i = 0; i < 5; i++) {
			LOG_INF("T_COOP busy %d/5", i + 1);
			k_busy_wait(1000);
		}
		LOG_INF("T_COOP yield");
		k_yield();
	}
}

K_THREAD_DEFINE(t_low, STACK_SIZE, t_low_fn, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(t_med, STACK_SIZE, t_med_fn, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(t_high, STACK_SIZE, t_high_fn, NULL, NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(t_coop, STACK_SIZE, t_coop_fn, NULL, NULL, NULL, -1, 0, 2000);

int main(void)
{
	return 0;
}
