/******************************************************************************
 * Filename:
 *   nc_gpio.c
 *
 * Description:
 *   nc_gpio sysfs API
 *
 * Author:
 *   gandy
 *
 * Version : V0.1_15-02-03
 * ---------------------------------------------------------------------------
 * Abbreviation
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "nc_gpio.h"
/******************************************************************************
 *
 * Variable Declaration
 *
 ******************************************************************************/
#define GPIO_SYSFS_PATH "/sys/class/gpio"
#define GPIO_SYSFS_EXPORT   GPIO_SYSFS_PATH"/export"

static int export_gpio(uint32_t num)
{
    int fd;
    int len;
    char buf[10] = {0,};

    if ((fd = open(GPIO_SYSFS_EXPORT, O_WRONLY)) < 0) {
        printf("<%s> file open failure(gpio%d)\n", __func__, num);
        return -1;
    }

    len = snprintf(buf, sizeof(buf), "%d", num);
    if (write(fd, buf, len) < 0) { 
        printf("<%s> file write failure(gpio%d)\n", __func__, num);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int set_gpio_dir(uint32_t num, uint32_t dir)
{
    int fd;
    int len;
    char buf[64] = {0,};

    len = snprintf(buf, sizeof(buf), GPIO_SYSFS_PATH"/gpio%d/direction", num);
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        printf("<%s> file open failure.\n", __func__);
        return -1;
    }

    len = snprintf(buf, sizeof(buf), "%s", (dir == GPIO_IN) ? "in" : "out");
    if (write(fd, buf, len) < 0) {
        printf("<%s> file write failure.\n", __func__);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int set_gpio_out(uint32_t num, uint32_t value)
{
    int fd;
    int len;
    char buf[64] = {0,};

    len = snprintf(buf, sizeof(buf), GPIO_SYSFS_PATH"/gpio%d/direction", num);
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        printf("<%s> file open failure.\n", __func__);
        return -1;
    }

    memset(buf, sizeof(buf), 0);
    len = snprintf(buf, sizeof(buf), "%s", (value == 1) ? "high" : "low");
    if (write(fd, buf, len) < 0) {
        printf("<%s> file write failure.\n", __func__);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int get_gpio_val(uint32_t num)
{
    int fd;
    int len;
    char buf[64] = {0,};

    len = snprintf(buf, sizeof(buf), GPIO_SYSFS_PATH"/gpio%d/value", num);
    fd = open(buf, O_RDWR);
    if (fd < 0) {
        printf("<%s> file open failure.\n", __func__);
        return -1;
    }

    len = read(fd, buf, sizeof(buf));
    if (len < 0) {
        printf("<%s> file read failure.\n", __func__);
        close(fd);
        return -1;
    }

    close(fd);
    return atoi(buf);
}

static int set_gpio_val(uint32_t num, int val)
{
    int fd;
    int len;
    char buf[64] = {0,};

    len = snprintf(buf, sizeof(buf), GPIO_SYSFS_PATH"/gpio%d/value", num); 
    fd = open(buf, O_RDWR);
    if (fd < 0) {
        printf("<%s> file open failure.\n", __func__);
        return -1;
    }

    len = snprintf(buf, sizeof(buf), "%d", val);
    if (write(fd, buf, len) < 0) {
        printf("<%s> file write failure.\n", __func__);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/*
    Export Functions.
*/
int nc_gpio_config(uint32_t port, uint32_t pin, uint32_t dir)
{
    uint32_t num;

    num = port + pin;
    export_gpio(num);
    return set_gpio_dir(num, dir);
}

void gpio_config_outvalue(uint32_t port, uint32_t pin, uint32_t val)
{
    uint32_t num;

    num = port + pin;
    export_gpio(num);
    set_gpio_out(num, val);
}

uint32_t nc_gpio_get(uint32_t port, uint32_t pin)
{
    uint32_t num;

    num = port + pin;
    if (get_gpio_val(num))
        return GPIO_HIGH;

    return GPIO_LOW;
}

int nc_gpio_set(uint32_t port, uint32_t pin)
{
    uint32_t num;

    num = port + pin;
    return set_gpio_val(num, 1);
}

int nc_gpio_clr(uint32_t port, uint32_t pin)
{
    uint32_t num;

    num = port + pin;
    return set_gpio_val(num, 0);
}
