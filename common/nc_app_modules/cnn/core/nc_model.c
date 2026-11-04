#include <stdint.h>
#include "nc_model.h"
#include "nc_cnn_config_parser.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define NC_MODEL_MAX_CLASS  (256)

/* names from network ini class_name that count as vehicle */
static const char *const k_vehicle_names[] = {
    "car",
    "bus",
    "tru", "truck",
    "mot", "motorcycle", "motorbike",
    "bik", "bike", "bicycle", "cycle",
};

typedef struct {
    int     built;
    int     class_num;
    uint8_t is_vehicle[NC_MODEL_MAX_CLASS];
} stModelTable;

static stModelTable    g_model[MAX_NETWORK_IDS];
static pthread_mutex_t g_model_lock = PTHREAD_MUTEX_INITIALIZER;

static int name_is_vehicle(const char *name)
{
    if (name == NULL) return 0;
    for (size_t i = 0; i < sizeof(k_vehicle_names) / sizeof(k_vehicle_names[0]); i++) {
        if (strcasecmp(name, k_vehicle_names[i]) == 0) return 1;
    }
    return 0;
}

/* caller holds g_model_lock */
static int build_table(E_NETWORK_UID net_id, stModelTable *t)
{
    stNetwork_info *info = nc_cnn_get_network_info(net_id);
    if (info == NULL || info->class_name == NULL || info->class_num <= 0) {
        return -1; /* not loaded yet: retry on next call */
    }

    int n = info->class_num;
    if (n > NC_MODEL_MAX_CLASS) {
        printf("[nc_model] net_id:%d class_num %d > %d, clipped\n", (int)net_id, n, NC_MODEL_MAX_CLASS);
        n = NC_MODEL_MAX_CLASS;
    }

    memset(t, 0, sizeof(*t));
    printf("[nc_model] net_id:%d vehicle classes:", (int)net_id);
    for (int i = 0; i < n; i++) {
        t->is_vehicle[i] = (uint8_t)name_is_vehicle(info->class_name[i]);
        if (t->is_vehicle[i]) printf(" %d(%s)", i, info->class_name[i]);
    }
    printf("\n");
    t->class_num = n;
    t->built = 1;
    return 0;
}

int nc_model_is_vehicle(E_NETWORK_UID net_id, int class_id)
{
    if ((int)net_id < 0 || (int)net_id >= (int)MAX_NETWORK_IDS || class_id < 0) return 0;

    int ret = 0;
    pthread_mutex_lock(&g_model_lock);
    stModelTable *t = &g_model[(int)net_id];
    if (t->built || build_table(net_id, t) == 0) {
        if (class_id < t->class_num) ret = t->is_vehicle[class_id];
    }
    pthread_mutex_unlock(&g_model_lock);
    return ret;
}

void nc_model_reset(void)
{
    pthread_mutex_lock(&g_model_lock);
    memset(g_model, 0, sizeof(g_model));
    pthread_mutex_unlock(&g_model_lock);
}
