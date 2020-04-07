#include <unistd.h>

#include <esp_nimble_hci.h>
#include <esp_timer.h>
#include <host/ble_gap.h>
#include <host/util/util.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <nvs_flash.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

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

static ble_uuid128_t service_uuid = UUID128_CONST(0xf6b35e8c, 0xb4a7, 0x4838, 0x8569, 0xe9091b9a8b0e);
static ble_uuid128_t timer_uuid   = UUID128_CONST(0x6952b350, 0x8d61, 0x4461, 0xb9c9, 0xb1ebfa70a838);
static ble_uuid16_t cpf_uuid	  = BLE_UUID16_INIT(0x2904);

static uint8_t addr_type;
static ble_gatt_access_fn read_timer_chr;
static ble_gatt_access_fn read_cpf_dsc;
static void advertise();

static const struct ble_gatt_svc_def service_list[] = {
	{
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = &service_uuid.u,
		.characteristics = (struct ble_gatt_chr_def[]){
			{
				.uuid = &timer_uuid.u,
				.access_cb = read_timer_chr,
				.descriptors = (struct ble_gatt_dsc_def[]){
					{
						.uuid = &cpf_uuid.u,
						.att_flags = BLE_ATT_F_READ,
						.access_cb = read_cpf_dsc,
					},
					{}, // End of descriptor list.
				},
				.flags = BLE_GATT_CHR_F_READ,
			},
			{}, // End of characteristic list.
		},
        },
	{}, // End of service list.
};

static void server_init(void) {
	int err;
	char buf[40];

	ble_uuid_to_str((ble_uuid_t *)&service_uuid, buf);
	printf("service UUID: %s\n", buf);

	ble_uuid_to_str((ble_uuid_t *)&timer_uuid, buf);
	printf("timer UUID: %s\n", buf);

	ble_svc_gap_init();
	ble_svc_gatt_init();

	err = ble_gatts_count_cfg(service_list);
	assert(!err);

	err = ble_gatts_add_svcs(service_list);
	assert(!err);
}

static void host_task(void *arg) {
	nimble_port_run();
}

static int handle_gap_event(struct ble_gap_event *e, void *arg) {
	switch (e->type) {
	case BLE_GAP_EVENT_CONNECT:
		if (e->connect.status != 0) {
			// Connection failed; resume advertising.
			printf("connection failed\n");
			advertise();
			return 0;
		}
		printf("connected\n");
		break;
	case BLE_GAP_EVENT_DISCONNECT:
		// Connection terminated; resume advertising.
		printf("disconnected\n");
		advertise();
		break;
	case BLE_GAP_EVENT_ADV_COMPLETE:
		printf("advertising complete\n");
		advertise();
		break;
	default:
		printf("GAP event %d\n", e->type);
		break;
	}
	return 0;
}

static void advertise(void) {
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

	printf("advertising started\n");
}

static void sync_callback(void) {
	int err;

	err = ble_hs_util_ensure_addr(0);
	assert(!err);

	err = ble_hs_id_infer_auto(0, &addr_type);
	assert(!err);

	uint8_t addr[6];
	ble_hs_id_copy_addr(addr_type, addr, 0);
	printf("device address: ");
	for (int i = 0; i < sizeof(addr); i++) {
		printf("%s%02x", i == 0 ? "" : ":", addr[i]);
	}
	printf("\n");

	advertise();
}

static int read_timer_chr(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	assert(ble_uuid_cmp(ctxt->chr->uuid, &timer_uuid.u) == 0);
	assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
	uint32_t ms = esp_timer_get_time() / 1000;
	printf("timer value = %u\n", ms);
	if (os_mbuf_append(ctxt->om, &ms, sizeof(ms)) != 0) {
		return BLE_ATT_ERR_INSUFFICIENT_RES;
	}
	return 0;
}

static int read_cpf_dsc(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	assert(ble_uuid_cmp(ctxt->dsc->uuid, &cpf_uuid.u) == 0);
	assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC);
	printf("read CPF descriptor\n");
	uint8_t data[7] = {
		[0] = 8, // format = uint32
		[4] = 1, // namespace = Bluetooth SIG Assigned Numbers
	};
	if (os_mbuf_append(ctxt->om, data, sizeof(data)) != 0) {
		return BLE_ATT_ERR_INSUFFICIENT_RES;
	}
	return 0;
}

void app_main(void) {
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());
	nimble_port_init();

	ble_hs_cfg.sync_cb = sync_callback;

	server_init();

	int err = ble_svc_gap_device_name_set("ESP32");
	assert(!err);

	ble_store_ram_init();
	nimble_port_freertos_init(host_task);
}
