#include <unistd.h>

#include "gnarl.h"

#include <esp_nimble_hci.h>
#include <esp_timer.h>
#include <host/ble_gap.h>
#include <host/util/util.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <nvs_flash.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

#include "display.h"

#define MAX_DATA	150

void ble_store_ram_init(void);

#define B0(x)	((x) & 0xFF)
#define B1(x)	(((x) >> 8) & 0xFF)
#define B2(x)	(((x) >> 16) & 0xFF)
#define B3(x)	(((x) >> 24) & 0xFF)
#define B4(x)	(((x) >> 32) & 0xFF)
#define B5(x)	(((x) >> 40) & 0xFF)

#define UUID128_CONST(a32, b16, c16, d16, e48)	\
	BLE_UUID128_INIT(						\
		B0(e48), B1(e48), B2(e48), B3(e48), B4(e48), B5(e48),	\
		B0(d16), B1(d16),					\
		B0(c16), B1(c16),					\
		B0(b16), B1(b16),					\
		B0(a32), B1(a32), B2(a32), B3(a32),			\
	)

static ble_uuid128_t service_uuid          = UUID128_CONST(0x0235733b, 0x99c5, 0x4197, 0xb856, 0x69219c2a3845);
static ble_uuid128_t data_uuid             = UUID128_CONST(0xc842e849, 0x5028, 0x42e2, 0x867c, 0x016adada9155);
static ble_uuid128_t response_count_uuid   = UUID128_CONST(0x6e6c7910, 0xb89e, 0x43a5, 0xa0fe, 0x50c5e2b81f4a);
static ble_uuid128_t timer_tick_uuid       = UUID128_CONST(0x6e6c7910, 0xb89e, 0x43a5, 0x78af, 0x50c5e2b86f7e);
static ble_uuid128_t custom_name_uuid      = UUID128_CONST(0xd93b2af0, 0x1e28, 0x11e4, 0x8c21, 0x0800200c9a66);
static ble_uuid128_t firmware_version_uuid = UUID128_CONST(0x30d99dc9, 0x7c91, 0x4295, 0xa051, 0x0a104d238cf2);
static ble_uuid128_t led_mode_uuid         = UUID128_CONST(0xc6d84241, 0xf1a7, 0x4f9c, 0xa25f, 0xfce16732f14e);

static ble_gap_event_fn handle_gap_event;
static uint8_t addr_type;
static ble_gatt_access_fn data_access;
static ble_gatt_access_fn custom_name_access;
static ble_gatt_access_fn led_mode_access;
static ble_gatt_access_fn firmware_version_access;
static ble_gatt_access_fn no_access;

static uint16_t connection_handle;

static uint16_t response_count_notify_handle;
static int response_count_notify_state;
static uint8_t response_count;

static uint16_t timer_tick_notify_handle;
static int timer_tick_notify_state;
static uint8_t timer_tick;
static void timer_tick_callback(void *);

static const struct ble_gatt_svc_def service_list[] = {
	{
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = &service_uuid.u,
		.characteristics = (struct ble_gatt_chr_def[]){
			{
				.uuid = &data_uuid.u,
				.access_cb = data_access,
				.flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
			},
			{
				.uuid = &response_count_uuid.u,
				.access_cb = no_access,
				.val_handle = &response_count_notify_handle,
				.flags = BLE_GATT_CHR_F_NOTIFY,
			},
			{
				.uuid = &timer_tick_uuid.u,
				.access_cb = no_access,
				.val_handle = &timer_tick_notify_handle,
				.flags = BLE_GATT_CHR_F_NOTIFY,
			},
			{
				.uuid = &custom_name_uuid.u,
				.access_cb = custom_name_access,
				.flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
			},
			{
				.uuid = &firmware_version_uuid.u,
				.access_cb = firmware_version_access,
				.flags = BLE_GATT_CHR_F_READ,
			},
			{
				.uuid = &led_mode_uuid.u,
				.access_cb = led_mode_access,
				.flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
			},
			{}, // End of characteristic list.
		},
        },
	{}, // End of service list.
};

static void server_init() {
	int err;
	char u[60];

	ble_svc_gap_init();
	ble_svc_gatt_init();

	err = ble_gatts_count_cfg(service_list);
	assert(!err);

	err = ble_gatts_add_svcs(service_list);
	assert(!err);

	ble_uuid_to_str(&service_uuid.u, u);
	ESP_LOGD(TAG, "service UUID %s", u);

	esp_timer_handle_t t;
	esp_timer_create_args_t timer_args = {
		.callback = timer_tick_callback,
	};
	ESP_ERROR_CHECK(esp_timer_create(&timer_args, &t));
	ESP_ERROR_CHECK(esp_timer_start_periodic(t, 60*SECONDS));
}

static void advertise() {
	struct ble_hs_adv_fields fields;
	memset(&fields, 0, sizeof(fields));

	fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

	fields.tx_pwr_lvl_is_present = 1;
	fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

	const char *name = ble_svc_gap_device_name();
	fields.name = (uint8_t *)name;
	fields.name_len = strlen(name);
	fields.name_is_complete = 1;

	fields.uuids128 = &service_uuid;
	fields.num_uuids128 = 1;
	fields.uuids128_is_complete = 1;

	int err = ble_gap_adv_set_fields(&fields);
	assert(!err);

	// Begin advertising.
	struct ble_gap_adv_params adv;
	memset(&adv, 0, sizeof(adv));
	adv.conn_mode = BLE_GAP_CONN_MODE_UND;
	adv.disc_mode = BLE_GAP_DISC_MODE_GEN;

	err = ble_gap_adv_start(addr_type, 0, BLE_HS_FOREVER, &adv, handle_gap_event, 0);
	assert(!err);

	ESP_LOGD(TAG, "advertising started");
}

static int handle_gap_event(struct ble_gap_event *e, void *arg) {
	switch (e->type) {
	case BLE_GAP_EVENT_CONNECT:
		if (e->connect.status != 0) {
			// Connection failed; resume advertising.
			ESP_LOGE(TAG, "connection failed");
			advertise();
			return 0;
		}
		connection_handle = e->connect.conn_handle;
		ESP_LOGI(TAG, "connected");
		ESP_LOGD(TAG, "connection handle %04X", connection_handle);
		ESP_LOGD(TAG, "response count notify handle %04X", response_count_notify_handle);
		ESP_LOGD(TAG, "timer tick notify handle %04X", timer_tick_notify_handle);
		break;
	case BLE_GAP_EVENT_DISCONNECT:
		// Connection terminated; resume advertising.
		ESP_LOGD(TAG, "disconnected");
		advertise();
		break;
	case BLE_GAP_EVENT_ADV_COMPLETE:
		ESP_LOGD(TAG, "advertising complete");
		advertise();
		break;
	case BLE_GAP_EVENT_SUBSCRIBE:
		if (e->subscribe.attr_handle == response_count_notify_handle) {
			ESP_LOGD(TAG, "notify %d for response count", e->subscribe.cur_notify);
			response_count_notify_state = e->subscribe.cur_notify;
			break;
		}
		if (e->subscribe.attr_handle == timer_tick_notify_handle) {
			ESP_LOGD(TAG, "notify %d for timer tick", e->subscribe.cur_notify);
			timer_tick_notify_state = e->subscribe.cur_notify;
			break;
		}
		ESP_LOGD(TAG, "notify %d for unknown handle %04X", e->subscribe.cur_notify, e->subscribe.attr_handle);
		break;
	default:
		ESP_LOGD(TAG, "GAP event %d", e->type);
		break;
	}
	return 0;
}

static void sync_callback() {
	int err;

	err = ble_hs_util_ensure_addr(0);
	assert(!err);

	err = ble_hs_id_infer_auto(0, &addr_type);
	assert(!err);

	uint8_t addr[6];
	ble_hs_id_copy_addr(addr_type, addr, 0);
	if (LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG) {
		printf("device address: ");
		for (int i = 0; i < sizeof(addr); i++) {
			printf("%s%02x", i == 0 ? "" : ":", addr[i]);
		}
		printf("\n");
	}
	advertise();
}

static uint8_t data_in[MAX_DATA];
static uint16_t data_in_len;

static uint8_t data_out[MAX_DATA];
static uint16_t data_out_len;

void send_bytes(const uint8_t *buf, int count) {
	print_bytes("send_bytes (%d):", buf, count);
	memcpy(data_out, buf, count);
	data_out[count++] = 0; // Terminate with 0 for Loop
	data_out_len = count;
	response_count++;
	if (!response_count_notify_state) {
		ESP_LOGD(TAG, "not notifying for response count %d", response_count);
		return;
	}
	struct os_mbuf *om = ble_hs_mbuf_from_flat(&response_count, sizeof(response_count));
	int err = ble_gattc_notify_custom(connection_handle, response_count_notify_handle, om);
	assert(!err);
	ESP_LOGD(TAG, "notify for response count %d", response_count);
}

void print_bytes(const char *msg, const uint8_t *buf, int count) {
	if (LOG_LOCAL_LEVEL < ESP_LOG_DEBUG) {
		return;
	}
	printf(msg, count);
	for (int i = 0; i < count; i++) {
		printf(" %02X", buf[i]);
	}
	printf("\n");
}

static void timer_tick_callback(void *arg) {
	timer_tick++;
	ESP_LOGD(TAG, "timer tick %d", timer_tick);
	if (!timer_tick_notify_state) {
		ESP_LOGD(TAG, "not notifying for timer tick");
		return;
	}
	struct os_mbuf *om = ble_hs_mbuf_from_flat(&timer_tick, sizeof(timer_tick));
	int err = ble_gattc_notify_custom(connection_handle, timer_tick_notify_handle, om);
	assert(!err);
	ESP_LOGD(TAG, "notify for timer tick");
}

static int data_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	int err;
	assert(ble_uuid_cmp(ctxt->chr->uuid, &data_uuid.u) == 0);
	switch (ctxt->op) {
	case BLE_GATT_ACCESS_OP_READ_CHR:
		print_bytes("data_access: sending %d bytes:", data_out, data_out_len);
		if (os_mbuf_append(ctxt->om, data_out, data_out_len) != 0) {
			return BLE_ATT_ERR_INSUFFICIENT_RES;
		}
		return 0;
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		err = ble_hs_mbuf_to_flat(ctxt->om, data_in, sizeof(data_in), &data_in_len);
		assert(!err);
		print_bytes("data_access: received %d bytes:", data_in, data_in_len);
		rfspy_command(data_in, data_in_len);
		return 0;
	default:
		assert(0);
	}
	return 0;
}

static uint8_t custom_name[30];
static uint16_t custom_name_len;

static int custom_name_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	int err;
	assert(ble_uuid_cmp(ctxt->chr->uuid, &data_uuid.u) == 0);
	switch (ctxt->op) {
	case BLE_GATT_ACCESS_OP_READ_CHR:
		print_bytes("custom_name_access: sending %d bytes:", custom_name, custom_name_len);
		if (os_mbuf_append(ctxt->om, custom_name, custom_name_len) != 0) {
			return BLE_ATT_ERR_INSUFFICIENT_RES;
		}
		return 0;
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		err = ble_hs_mbuf_to_flat(ctxt->om, custom_name, sizeof(custom_name), &custom_name_len);
		assert(!err);
		print_bytes("custom_name_access: received %d bytes:", custom_name, custom_name_len);
		return 0;
	default:
		assert(0);
	}
	return 0;
}

static int firmware_version_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	assert(ble_uuid_cmp(ctxt->chr->uuid, &firmware_version_uuid.u) == 0);
	assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
	ESP_LOGD(TAG, "BLE firmware version = %s", BLE_RFSPY_VERSION);
	if (os_mbuf_append(ctxt->om, (const uint8_t *)BLE_RFSPY_VERSION, strlen(BLE_RFSPY_VERSION)) != 0) {
		return BLE_ATT_ERR_INSUFFICIENT_RES;
	}
	return 0;
}

static uint8_t led_mode;

static int led_mode_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	int err;
	uint16_t n;
	assert(ble_uuid_cmp(ctxt->chr->uuid, &data_uuid.u) == 0);
	switch (ctxt->op) {
	case BLE_GATT_ACCESS_OP_READ_CHR:
		ESP_LOGD(TAG, "led_mode_access: mode = %d", led_mode);
		if (os_mbuf_append(ctxt->om, &led_mode, sizeof(led_mode))) {
			return BLE_ATT_ERR_INSUFFICIENT_RES;
		}
		return 0;
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		err = ble_hs_mbuf_to_flat(ctxt->om, &led_mode, sizeof(led_mode), &n);
		assert(!err);
		ESP_LOGD(TAG, "led_mode_access: set mode = %d", led_mode);
		return 0;
	default:
		assert(0);
	}
	return 0;
}

static int no_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	char u[60];
	ble_uuid_to_str(ctxt->chr->uuid, u);
	ESP_LOGE(TAG, "should not happen: op %d, attr handle %04X, uuid %s", ctxt->op, attr_handle, u);
	return 0;
}

static void host_task(void *arg) {
	nimble_port_run();
}

void gnarl_init() {
	start_gnarl_task();

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());
	nimble_port_init();

	ble_hs_cfg.sync_cb = sync_callback;

	server_init();

	int err = ble_svc_gap_device_name_set("GNARL");
	assert(!err);

	ble_store_ram_init();
	nimble_port_freertos_init(host_task);
}
