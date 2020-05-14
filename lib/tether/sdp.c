#define TAG		"SDP"
#define LOG_LOCAL_LEVEL	ESP_LOG_INFO
#include <esp_log.h>

#include <btstack_config.h>
#include <btstack.h>

bd_addr_t bt_tether_addr;
volatile int bnep_failure;

static uint16_t bnep_l2cap_psm = 0;
static uint32_t bnep_remote_uuid = 0;

void handle_bnep_packet(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static void handle_sdp_query_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
	uint16_t off, len, id;
	uint8_t data;
	des_iterator_t des_list_it;
	static uint8_t attribute_value[512];

	switch (hci_event_packet_get_type(packet)) {
	case SDP_EVENT_QUERY_ATTRIBUTE_VALUE:
		off = sdp_event_query_attribute_byte_get_data_offset(packet);
		len = sdp_event_query_attribute_byte_get_attribute_length(packet);
		if (len > sizeof(attribute_value)) {
			ESP_LOGE(TAG, "length %d overflows attribute buffer", len);
			break;
		}
		if (off >= sizeof(attribute_value)) {
			ESP_LOGE(TAG, "invalid attribute data offset %d", off);
			break;
		}
		data = sdp_event_query_attribute_byte_get_data(packet);
		attribute_value[off] = data;
		if (off < len - 1) {
			break;
		}
		id = sdp_event_query_attribute_byte_get_attribute_id(packet);
		switch (id) {
		case BLUETOOTH_ATTRIBUTE_SERVICE_CLASS_ID_LIST:
			if (de_get_element_type(attribute_value) != DE_DES) {
				break;
			}
			for (des_iterator_init(&des_list_it, attribute_value); des_iterator_has_more(&des_list_it); des_iterator_next(&des_list_it)) {
				uint8_t *element = des_iterator_get_element(&des_list_it);
				if (de_get_element_type(element) != DE_UUID) {
					continue;
				}
				uint32_t uuid = de_get_uuid32(element);
				switch (uuid) {
				case BLUETOOTH_SERVICE_CLASS_PANU:
				case BLUETOOTH_SERVICE_CLASS_NAP:
				case BLUETOOTH_SERVICE_CLASS_GN:
					bnep_remote_uuid = uuid;
					ESP_LOGI(TAG, "BNEP remote uuid = %04x", bnep_remote_uuid);
					break;
				}
			}
			break;
		case BLUETOOTH_ATTRIBUTE_PROTOCOL_DESCRIPTOR_LIST:
			for (des_iterator_init(&des_list_it, attribute_value); des_iterator_has_more(&des_list_it); des_iterator_next(&des_list_it)) {
				if (des_iterator_get_type(&des_list_it) != DE_DES) {
					continue;
				}
				uint8_t *des_element = des_iterator_get_element(&des_list_it);
				des_iterator_t prot_it;
				des_iterator_init(&prot_it, des_element);
				uint8_t *element = des_iterator_get_element(&prot_it);
				if (!element) {
					continue;
				}
				if (de_get_element_type(element) != DE_UUID) {
					continue;
				}
				uint32_t uuid = de_get_uuid32(element);
				des_iterator_next(&prot_it);
				if (uuid == BLUETOOTH_PROTOCOL_L2CAP) {
					if (!des_iterator_has_more(&prot_it)) {
						continue;
					}
					de_element_get_uint16(des_iterator_get_element(&prot_it), &bnep_l2cap_psm);
					ESP_LOGI(TAG, "L2CAP PSM = %04x", bnep_l2cap_psm);
				}
			}
			break;
		}
		break;
	case SDP_EVENT_QUERY_COMPLETE:
		if (!bnep_l2cap_psm) {
			bnep_failure = 1;
			ESP_LOGE(TAG, "BNEP service not found");
			break;
		}
		ESP_LOGI(TAG, "discovered BNEP service");
		bnep_connect(handle_bnep_packet, bt_tether_addr, bnep_l2cap_psm, BLUETOOTH_SERVICE_CLASS_PANU, bnep_remote_uuid);
		break;
	}
}

void handle_hci_startup_packet(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
	if (packet_type != HCI_EVENT_PACKET) {
		return;
	}
	if (hci_event_packet_get_type(packet) != BTSTACK_EVENT_STATE) {
		return;
	}
	if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) {
		return;
	}
	ESP_LOGI(TAG, "SDP query for remote PAN access point");
	bnep_failure = 0;
	sdp_client_query_uuid16(handle_sdp_query_result, bt_tether_addr, BLUETOOTH_SERVICE_CLASS_NAP);
}
