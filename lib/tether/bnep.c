#define TAG		"BNEP"
#define LOG_LOCAL_LEVEL	ESP_LOG_INFO
#include <esp_log.h>

#include <btstack_config.h>
#include <btstack.h>
#include <btstack_run_loop_freertos.h>
#include <btstack_port_esp32.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/dhcp.h>
#include <lwip/etharp.h>
#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <netif/ethernet.h>

#include "network_config.h"

extern volatile int bnep_failure;

static struct netif bnep_netif;

static uint16_t bnep_cid;

static QueueHandle_t outgoing_queue;

static struct pbuf *next_packet;	// only modified from btstack context

static void netif_link_up(bd_addr_t network_address) {
	memcpy(bnep_netif.hwaddr, network_address, BD_ADDR_LEN);
	bnep_netif.flags |= NETIF_FLAG_LINK_UP;
	netif_set_up(&bnep_netif);
}

static void netif_link_down(void) {
	bnep_netif.flags &= ~NETIF_FLAG_LINK_UP;
	netif_set_down(&bnep_netif);
}

static void packet_processed(void) {
	pbuf_free_callback(next_packet);
	next_packet = 0;
}

static void handle_outgoing(void *unused) {
	if (next_packet) {
		ESP_LOGD(TAG, "handle_outgoing: previous packet not yet sent");
		return;
	}
	xQueueReceive(outgoing_queue, &next_packet, portMAX_DELAY);
	bnep_request_can_send_now_event(bnep_cid);
}

static void trigger_outgoing_process(void) {
	btstack_run_loop_freertos_execute_code_on_main_thread(handle_outgoing, 0);
}

static void send_next_packet(void) {
	if (!next_packet) {
		ESP_LOGE(TAG, "send_next_packet: no packet queued");
		return;
	}
	static uint8_t buffer[HCI_ACL_PAYLOAD_SIZE];
	uint32_t len = btstack_min(sizeof(buffer), next_packet->tot_len);
	pbuf_copy_partial(next_packet, buffer, len, 0);
	ESP_LOGD(TAG, "send_next_packet: bnep_send %d bytes", len);
	bnep_send(bnep_cid, buffer, len);
	packet_processed();
	if (uxQueueMessagesWaiting(outgoing_queue) != 0) {
		trigger_outgoing_process();
	}
}

static void discard_packets(void) {
	if (next_packet) {
		packet_processed();
	}
	xQueueReset(outgoing_queue);
}

static void receive_packet(const uint8_t *packet, uint16_t size) {
	ESP_LOGD(TAG, "receive_packet: %d bytes", size);
	struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
	if (p == 0) {
		ESP_LOGE(TAG, "receive_packet: pbuf_alloc failed");
		return;
	}
	struct pbuf *q = p;
	while (q && size) {
		memcpy(q->payload, packet, q->len);
		packet += q->len;
		size -= q->len;
		q = q->next;
	}
	if (size != 0) {
		ESP_LOGE(TAG, "receive_packet: %d bytes remaining after copying packet into pbuf", size);
		pbuf_free_callback(p);
		return;
	}
	int r = bnep_netif.input(p, &bnep_netif);
	if (r != ERR_OK) {
		ESP_LOGE(TAG, "receive_packet: IP input error %d", r);
		pbuf_free_callback(p);
	}
}

void handle_bnep_packet(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
	bd_addr_t local_addr;
	switch (packet_type) {
        case HCI_EVENT_PACKET:
		switch (hci_event_packet_get_type(packet)) {
                case BNEP_EVENT_CHANNEL_OPENED:
			if (bnep_event_channel_opened_get_status(packet) != 0) {
				break;
			}
			bnep_cid = bnep_event_channel_opened_get_bnep_cid(packet);
			gap_local_bd_addr(local_addr);
			netif_link_up(local_addr);
			ESP_LOGD(TAG, "BNEP channel opened: CID = %x", bnep_cid);
			break;
                case BNEP_EVENT_CHANNEL_CLOSED:
			bnep_failure = 1;
			bnep_cid = 0;
			discard_packets();
			netif_link_down();
			ESP_LOGD(TAG, "BNEP channel closed");
			break;
                case BNEP_EVENT_CAN_SEND_NOW:
			send_next_packet();
			break;
		}
		break;
        case BNEP_DATA_PACKET:
		receive_packet(packet, size);
		break;
	}
}

static err_t link_output(struct netif *netif, struct pbuf *p) {
	ESP_LOGD(TAG, "link_output: length = %d, total = %d", p->len, p->tot_len);
	if (bnep_cid == 0) {
		ESP_LOGD(TAG, "link_output: BNEP CID = 0");
		return ERR_OK;
	}
	pbuf_ref(p);
	int queue_empty = uxQueueMessagesWaiting(outgoing_queue) == 0;
	xQueueSendToBack(outgoing_queue, &p, portMAX_DELAY);
	if (queue_empty) {
		trigger_outgoing_process();
	}
	return ERR_OK;
}

static err_t bnep_netif_init(struct netif *netif) {
	netif->name[0] = 'b';
	netif->name[1] = 't';
	netif->hwaddr_len = BD_ADDR_LEN;
	netif->mtu = 1600;
	netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_UP;
	netif->output = etharp_output;
	netif->linkoutput = link_output;
	return ERR_OK;
}

static void bnep_interface_init(void) {
	tcpip_init(0, 0);

	ip4_addr_t ipaddr, netmask, gw;
	IP4_ADDR(&ipaddr, 0U, 0U, 0U, 0U);
	IP4_ADDR(&netmask, 0U, 0U, 0U, 0U);
	IP4_ADDR(&gw, 0U, 0U, 0U, 0U);

	outgoing_queue = xQueueCreate(TCP_SND_QUEUELEN, sizeof(struct pbuf *));
	if (outgoing_queue == 0) {
		ESP_LOGE(TAG, "cannot allocate outgoing queue");
	}

	netif_add(&bnep_netif, &ipaddr, &netmask, &gw, 0, bnep_netif_init, ethernet_input);
	netif_set_default(&bnep_netif);
}

static int dhcp_started = 0;

static void link_callback(struct netif *netif) {
	if (netif_is_link_up(netif) && !dhcp_started) {
		dhcp_started = 1;
		ESP_LOGD(TAG, "starting DHCP");
		dhcp_start(netif);
	}
}

static volatile int have_ip_address = 0;
static char ip_addr_str[IP4ADDR_STRLEN_MAX];
static char gw_addr_str[IP4ADDR_STRLEN_MAX];

const char *ip_address(void) {
	return ip_addr_str;
}

const char *gateway_address(void) {
	return gw_addr_str;
}

static void status_callback(struct netif *netif) {
	struct ip4_addr *ip = &netif->ip_addr.u_addr.ip4;
	if (ip->addr == 0) {
		return;
	}
	ip4addr_ntoa_r(ip, ip_addr_str, sizeof(ip_addr_str));
	ip = &netif->gw.u_addr.ip4;
	ip4addr_ntoa_r(ip, gw_addr_str, sizeof(gw_addr_str));
	have_ip_address = 1;
}

#define WAIT_INTERVAL	100	// milliseconds
#define MAX_BNEP_WAITS	3000	// 5 minutes
#define MAX_DHCP_WAITS	1200	// 2 minutes

static int wait_for_dhcp(void) {
	int n = 0;
	while (!dhcp_started && !bnep_failure && n < MAX_BNEP_WAITS) {
		link_callback(&bnep_netif);
		if (n++ % 50 == 0) {
			ESP_LOGD(TAG, "waiting for BNEP connection");
		}
		vTaskDelay(pdMS_TO_TICKS(WAIT_INTERVAL));
	}
	if (n == MAX_BNEP_WAITS) {
		ESP_LOGE(TAG, "timeout waiting for BNEP connection");
		return -1;
	}
	n = 0;
	while (!have_ip_address && !bnep_failure && n < MAX_DHCP_WAITS) {
		status_callback(&bnep_netif);
		if (n++ % 50 == 0) {
			ESP_LOGD(TAG, "waiting for IP address");
		}
		vTaskDelay(pdMS_TO_TICKS(WAIT_INTERVAL));
	}
	if (n == MAX_DHCP_WAITS) {
		ESP_LOGE(TAG, "timeout waiting for IP address");
		return -1;
	}
	return have_ip_address ? 0 : -1;
}

extern bd_addr_t bt_tether_addr;

void handle_hci_startup_packet(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

// See https://www.bluetooth.com/specifications/assigned-numbers/baseband/
#define SERVICE_CLASS_NETWORKING	(1 << 17)
#define DEVICE_CLASS_LAN_NAP		(3 << 8)

static void bt_loop(void *unused) {
	btstack_init();
	// Parse human-readable Bluetooth address.
	sscanf_bd_addr(TETHER_ADDRESS, bt_tether_addr);
	gap_discoverable_control(1);
	gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
	gap_set_local_name("ESP32 PAN Client");
	gap_set_class_of_device(SERVICE_CLASS_NETWORKING | DEVICE_CLASS_LAN_NAP);
	l2cap_init();
	sdp_init();
	bnep_init();
	bnep_interface_init();
	static btstack_packet_callback_registration_t hci_callback = {
		.callback = handle_hci_startup_packet,
	};
	hci_add_event_handler(&hci_callback);
	hci_power_control(HCI_POWER_ON);
	btstack_run_loop_execute();
}

int tether_init(void) {
	xTaskCreate(bt_loop, "bt_loop", 4096, 0, ESP_TASK_BT_CONTROLLER_PRIO, 0);
	return wait_for_dhcp();
}

void tether_off(void) {
	bnep_disconnect(bt_tether_addr);
}
