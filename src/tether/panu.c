#define TAG		"PANU"
#define LOG_LOCAL_LEVEL	ESP_LOG_INFO
#include <esp_log.h>
#include <esp_event.h>

#include <btstack_config.h>
#include <btstack.h>

#include "bnep_config.h"

bd_addr_t remote_addr;

void handle_hci_startup_packet(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

void bnep_interface_init(void);

void main_loop(void *);

static void panu_setup(void) {
	static btstack_packet_callback_registration_t hci_callback = {
		.callback = handle_hci_startup_packet,
	};
	// Parse human-readable Bluetooth address.
	sscanf_bd_addr(BNEP_ADDRESS, remote_addr);
	gap_discoverable_control(1);
	gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
	gap_set_local_name("PANU Client 00:00:00:00:00:00");
	gap_set_class_of_device(0x020300); // service class "Networking", major device class "LAN / NAT"
	hci_add_event_handler(&hci_callback);
	l2cap_init();
	bnep_init();
	sdp_init();
	bnep_interface_init();
}

int btstack_main(int argc, const char *argv[]) {
	panu_setup();
	hci_power_control(HCI_POWER_ON);
	xTaskCreate(main_loop, "main", 4096, 0, tskIDLE_PRIORITY + 16, 0);
	return 0;
}
