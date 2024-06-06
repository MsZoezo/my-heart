#include "setup.h"

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

// const char* device_name = "zoeys-heart";

// UUID for our gatt service: f5042e1c-2731-460a-87b8-34a918c062f3
static const ble_uuid128_t gatt_svc_uuid = BLE_UUID128_INIT(0xf3, 0x62, 0xc0, 0x18, 0xa9, 0x34, 0xb8, 0x87, 0x0a, 0x46, 0x31, 0x27, 0x1c, 0x2e, 0x04, 0xf5);

// UUID for our gatt service characteristic: 6d012e2f-64f8-4730-95fb-fabb30dfa6cd
static const ble_uuid128_t gatt_chr_uuid = BLE_UUID128_INIT(0xcd, 0xa6, 0xdf, 0x30, 0xbb, 0xfa, 0xfb, 0x95, 0x30, 0x47, 0xf8, 0x64, 0x2f, 0x2e, 0x01, 0x6d);

static uint8_t own_addr_type;

// ----- Forward declarations -----

// When our characteristic is accessed we run this.
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

static int gap_event(struct ble_gap_event *event, void *arg);

static void advertise();

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_chr_uuid.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ
            },

            {
                0,
            },
        },
    },

    {
        0,
    },
};

static void on_reset(int reason)
{
    printf("Resetting state; reason=%d\n", reason);
}

static void on_sync(void)
{
    printf("syncing?\n");
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0)
    {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    advertise();
}

void host_task(void *param)
{
    nimble_port_run();

    nimble_port_freertos_deinit();
}

static void advertise() {
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

    if (rc != 0) {
        printf("error setting advertisement data; rc=%d\n", rc);
        return;
    }

    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    /* Begin advertising. */
    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, 0, BLE_HS_FOREVER, &adv_params, gap_event, NULL);
    if (rc != 0) {
        printf("error enabling advertisement; rc=%d\n", rc);
        return;
    }

    printf("did we ever get here?\n");
}

static int gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        printf("connection %s; status=%d ", event->connect.status == 0 ? "established" : "failed", event->connect.status);
        if (event->connect.status == 0) {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);

            if(rc != 0) return rc;
            // bleprph_print_conn_desc(&desc);
        }

        if (event->connect.status != 0) {
            advertise();
        }

        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        printf("disconnect; reason=%d ", event->disconnect.reason);
        // bleprph_print_conn_desc(&event->disconnect.conn);

        advertise();
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        printf("connection updated; status=%d ", event->conn_update.status);
        rc = ble_gap_conn_find(event->connect.conn_handle, &desc);

        if(rc != 0) return rc;
        // bleprph_print_conn_desc(&desc);
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        printf("encryption change event; status=%d ",
                    event->enc_change.status);
        rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
        assert(rc == 0);
        // bleprph_print_conn_desc(&desc);
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        printf("subscribe event; conn_handle=%d attr_handle=%d reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                    event->subscribe.conn_handle,
                    event->subscribe.attr_handle,
                    event->subscribe.reason,
                    event->subscribe.prev_notify,
                    event->subscribe.cur_notify,
                    event->subscribe.prev_indicate,
                    event->subscribe.cur_indicate);
        return 0;
    }

    return 0;
}

bool setup_should_run() {
    return true;
}

void setup_run() {
    int rc;
    esp_err_t ret;

    ret = nimble_port_init();

    if(ret != ESP_OK) {
        printf("Failed to init nimble: %d\n", ret);
        return;
    }

    rc = esp_nimble_hci_init();
    if (rc != ESP_OK) {
        printf("esp_nimble_hci_init() failed with error: %d", rc);
        return;
    }


    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;


    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_svc_gap_device_name_set("zoeys-heart");

    if(rc != 0) return;

    // Register our service.
    ble_gatts_count_cfg(gatt_svr_svcs);
    ble_gatts_add_svcs(gatt_svr_svcs);

    nimble_port_freertos_init(host_task);
}

static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    // The UUID of the characteristic that's being accessed.
    const ble_uuid_t* uuid = ctxt->chr->uuid;
    int rc;

    // If it's our characteristic uuid.
    if(ble_uuid_cmp(uuid, &gatt_chr_uuid.u) == 0) {
        switch(ctxt->op) {
            case BLE_GATT_ACCESS_OP_READ_CHR:
                rc = os_mbuf_append(ctxt->om, "HELLO WORLD!", sizeof("HELLO WORLD!"));
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

            default:
                return BLE_ATT_ERR_UNLIKELY;
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}