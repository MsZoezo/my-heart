#include <bluetooth.h>

static uint8_t own_addr_type;

// Forward definitions
static void bluetooth_on_sync();
void bluetooth_host_task();
static void bluetooth_advertise();
static int bluetooth_gap_event(struct ble_gap_event *event, void *arg);

int bluetooth_add_gatt_services(const struct ble_gatt_svc_def *services) {
    int rc;

    rc = ble_gatts_count_cfg(services);
    if(rc != 0) return rc;

    rc = ble_gatts_add_svcs(services);
    return rc;
}

int bluetooth_init(const char *name) {
    int rc;

    rc = nimble_port_init();
    if(rc != 0) return rc;

    rc = esp_nimble_hci_init();
    if(rc != 0) return rc;

    ble_hs_cfg.sync_cb = bluetooth_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_svc_gap_device_name_set(name);
    if(rc != 0) return rc;

    return 0;
}

void bluetooth_run() {
    nimble_port_freertos_init(bluetooth_host_task);
}

int bluetooth_stop() {
    int rc;

    rc = nimble_port_stop();
    if(rc != 0) return rc;

    nimble_port_deinit();

    rc = esp_nimble_hci_deinit();
    if(rc != 0) return rc;

    return 0;
}

int bluetooth_characteristic_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len, void *dst, uint16_t *len) {
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) return BLE_ATT_ERR_UNLIKELY;

    return 0;
}

static void bluetooth_on_sync() {
    int rc;

    rc = ble_hs_util_ensure_addr(0);

    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) return;

    bluetooth_advertise();
}

void bluetooth_host_task() {
    nimble_port_run();

    nimble_port_freertos_deinit();
}

static void bluetooth_advertise() {
    struct ble_hs_adv_fields fields;
    struct ble_gap_adv_params adv_params;

    int rc;

    const char* name = ble_svc_gap_device_name();

    memset(&fields, 0, sizeof fields);

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *) name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    fields.uuids16 = (ble_uuid16_t[]){BLE_UUID16_INIT(0x1811)};
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = true;

    rc = ble_gap_adv_set_fields(&fields);

    if (rc != 0) return;
    
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(own_addr_type, 0, BLE_HS_FOREVER, &adv_params, bluetooth_gap_event, NULL);
    if (rc != 0) return;
}

static int bluetooth_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);

            return rc;
        }

        if (event->connect.status != 0) {
            bluetooth_advertise();
        }

        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        bluetooth_advertise();
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
        return rc;

    case BLE_GAP_EVENT_ENC_CHANGE:
        rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
        return rc;

    case BLE_GAP_EVENT_SUBSCRIBE:
        return 0;
    }

    return 0;
}