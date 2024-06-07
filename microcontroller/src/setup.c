#include <setup.h>
#include <bluetooth.h>
#include <wifi.h>

char setup_ssid[100];
bool setup_ssid_set = false;
char setup_password[100];
bool setup_password_set = false;

// UUID for our gatt service: f5042e1c-2731-460a-87b8-34a918c062f3
static const ble_uuid128_t setup_gatt_svc_uuid = BLE_UUID128_INIT(0xf3, 0x62, 0xc0, 0x18, 0xa9, 0x34, 0xb8, 0x87, 0x0a, 0x46, 0x31, 0x27, 0x1c, 0x2e, 0x04, 0xf5);

// UUID for our SSID gatt characteristic: 6d012e2f-64f8-4730-95fb-fabb30dfa6cd
static const ble_uuid128_t setup_gatt_ssid_chr_uuid = BLE_UUID128_INIT(0xcd, 0xa6, 0xdf, 0x30, 0xbb, 0xfa, 0xfb, 0x95, 0x30, 0x47, 0xf8, 0x64, 0x2f, 0x2e, 0x01, 0x6d);

// UUID for our password gatt characteristic: e8657ffc-b1c9-4307-8a4c-fe6ca2f5216d
static const ble_uuid128_t setup_gatt_pwd_chr_uuid = BLE_UUID128_INIT(0x6d, 0x21, 0xf5, 0xa2, 0x6c, 0xfe, 0x4c, 0x8a, 0x07, 0x43, 0xc9, 0xb1, 0xfc, 0x7f, 0x65, 0xe8); 

// When our characteristic are accessed we run this.
static int setup_characteristic_accessed(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &setup_gatt_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &setup_gatt_ssid_chr_uuid.u,
                .access_cb = setup_characteristic_accessed,
                .flags = BLE_GATT_CHR_F_WRITE
            },

            {
                .uuid = &setup_gatt_pwd_chr_uuid.u,
                .access_cb = setup_characteristic_accessed,
                .flags = BLE_GATT_CHR_F_WRITE
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

bool setup_should_run() {
    return true;
}

void setup_run() {
    bluetooth_init("Zoey's Heart");
    bluetooth_add_gatt_services(gatt_svr_svcs);
    bluetooth_run();

    while(!setup_password_set || !setup_ssid_set) {
        printf("Awaiting wifi SSID & password...\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    bluetooth_stop();

    wifi_connect(setup_ssid, setup_password);
    
    printf("%s - %s\n", setup_ssid, setup_password);
}

static int setup_characteristic_accessed(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    // The UUID of the characteristic that's being accessed.
    const ble_uuid_t* uuid = ctxt->chr->uuid;
    int rc;

    // If the UUID is for the SSID.
    if(ble_uuid_cmp(uuid, &setup_gatt_ssid_chr_uuid.u) == 0) {
        switch(ctxt->op) {
            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                rc = bluetooth_characteristic_write(ctxt->om, 1, sizeof setup_ssid, &setup_ssid, NULL);
                if(rc == 0) setup_ssid_set = true;
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

            default:
                return BLE_ATT_ERR_UNLIKELY;
        }
    }

    // If the UUID is for the password.
    if(ble_uuid_cmp(uuid, &setup_gatt_pwd_chr_uuid.u) == 0) {
        switch(ctxt->op) {
            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                rc = bluetooth_characteristic_write(ctxt->om, 1, sizeof setup_password, &setup_password, NULL);
                if(rc == 0) setup_password_set = true;
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

            default:
                return BLE_ATT_ERR_UNLIKELY;
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}