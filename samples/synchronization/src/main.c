#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sync_demo, LOG_LEVEL_INF);

#define STACK_SIZE 1024

K_MUTEX_DEFINE(mutex);
static uint32_t shared_counter = 0;

static void t_first_fn(void *a, void *b, void *c)
{
	while (1) {
		k_mutex_lock(&mutex, K_FOREVER);
		LOG_INF("First thread acquired the mutex!");
		uint32_t local = shared_counter;
		LOG_INF("First thread sees counter as: %d", local);
		k_yield();
		shared_counter = local + 1;
		k_mutex_unlock(&mutex);
		// k_msleep(10);
	}
}

static void t_second_fn(void *a, void *b, void *c)
{
	while (1) {
		k_mutex_lock(&mutex, K_FOREVER);
		LOG_INF("Second thread acquired the mutex!");
		uint32_t local = shared_counter;
		LOG_INF("Second thread sees counter as: %d", local);
		k_yield();
		shared_counter = local + 1;
		k_mutex_unlock(&mutex);
		// k_msleep(10);
	}
}


K_THREAD_DEFINE(t_first, STACK_SIZE, t_first_fn, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(t_second, STACK_SIZE, t_second_fn, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	return 0;
}
