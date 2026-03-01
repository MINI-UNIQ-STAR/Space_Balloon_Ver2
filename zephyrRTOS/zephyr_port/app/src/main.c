#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(space_balloon_zephyr, LOG_LEVEL_INF);

int main(void)
{
    printk("Hello from Zephyr Space_Balloon_Ver2\\n");
    LOG_INF("Space_Balloon_Ver2 Zephyr skeleton booted");

    while (1) {
        LOG_INF("Periodic heartbeat: app alive");
        k_msleep(1000);
    }

    return 0;
}
