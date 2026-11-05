#ifndef NC_CLASS_H
#define NC_CLASS_H

/*
 * nc_class : model-independent class queries
 *
 * Class index order differs per model ini
 *   (e.g. coco: per,bik,car,... / ncdl: per,car,bus,...),
 * so class ids must not be hard-coded. Classes are classified by the
 * class_name loaded from the network ini (case-insensitive exact match).
 *
 * Vehicle names: car, bus, tru/truck, mot/motorcycle/motorbike,
 *                bik/bike/bicycle/cycle
 */

#include "nc_cnn_network_includes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 1 if class_id of net_id is a vehicle (trajectory target), else 0.
 * Returns 0 for invalid net_id / class_id or not-loaded network. */
int nc_class_is_vehicle(E_NETWORK_UID net_id, int class_id);

/* drop cached class tables (call after network ini is reloaded) */
void nc_class_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* NC_CLASS_H */
