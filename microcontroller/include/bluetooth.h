#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <esp_nimble_hci.h>
#include <nimble/ble.h>
#include <nimble/nimble_opt.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <host/ble_hs.h>
#include <host/util/util.h>
#include <host/ble_uuid.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

int bluetooth_add_gatt_services(const struct ble_gatt_svc_def *services);

int bluetooth_init(const char *name);
void bluetooth_run();
int bluetooth_stop();

int bluetooth_characteristic_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len, void *dst, uint16_t *len);

#endif