#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

extern "C" {
#include "display.h"
#include "gnarl.h"
}

#define RL_SERVICE_UUID "0235733b-99c5-4197-b856-69219c2a3845"

#define RL_DATA_CH		"c842e849-5028-42e2-867c-016adada9155"
#define RL_RESPONSE_COUNT_CH	"6e6c7910-b89e-43a5-a0fe-50c5e2b81f4a"
#define RL_TIMER_TICK_CH	"6e6c7910-b89e-43a5-78af-50c5e2b86f7e"
#define RL_CUSTOM_NAME_CH	"d93b2af0-1e28-11e4-8c21-0800200c9a66"
#define RL_FIRMWARE_VERSION_CH	"30d99dc9-7c91-4295-a051-0a104d238cf2"
#define RL_LED_MODE_CH		"c6d84241-f1a7-4f9c-a25f-fce16732f14e"

static BLECharacteristic* data_ch;
static BLECharacteristic* response_count_ch;
static BLECharacteristic* firmware_version_ch;

static uint8_t response_count;

static const uint32_t READ   = BLECharacteristic::PROPERTY_READ;
static const uint32_t WRITE  = BLECharacteristic::PROPERTY_WRITE;
static const uint32_t NOTIFY = BLECharacteristic::PROPERTY_NOTIFY;

void send_bytes(uint8_t* buf, int count) {
	if (count > 0 && buf[count-1] == 0) {
		count--;
	}
	if (count == 0) {
		ESP_LOGE(TAG, "send_bytes: count = 0");
	}
	data_ch->setValue(buf, count);
	response_count++;
	response_count_ch->setValue(&response_count, 1);
	response_count_ch->notify();
}

class data_callbacks: public BLECharacteristicCallbacks {
#if 0
	void onRead(BLECharacteristic *ch) {
		ESP_LOGD(TAG, "onRead(Data)");
	}
#endif
	void onWrite(BLECharacteristic *ch) {
		rfspy_command(ch->getData(), ch->getValue().length());
	}
};

class server_callbacks: public BLEServerCallbacks {
	void onConnect(BLEServer* srv, esp_ble_gatts_cb_param_t *p) {
		ESP_LOGD(TAG, "connected");
		char buf[3 * ESP_BD_ADDR_LEN];
		char *bp = buf;
		for (int i = 0; i < ESP_BD_ADDR_LEN; i++, bp += 2) {
			if (i != 0) {
				*bp++ = ':';
			}
			sprintf(bp, "%02X", p->connect.remote_bda[i]);
		}
		ESP_LOGD(TAG, "remote BDA %s", buf);
		display_update(CONNECTED, true);
	}

	void onDisconnect(BLEServer* srv) {
		ESP_LOGD(TAG, "disconnected");
		display_update(CONNECTED, false);
	}
};

void gnarl_init() {
	start_gnarl_task();

	BLEDevice::init("GNARL");
	BLEDevice::setPower(ESP_PWR_LVL_N12);

	BLEServer* srv = BLEDevice::createServer();
	srv->setCallbacks(new server_callbacks);
	BLEService* svc = srv->createService(RL_SERVICE_UUID);

	data_ch = svc->createCharacteristic(RL_DATA_CH, READ | WRITE);
	data_ch->setCallbacks(new data_callbacks);

	response_count_ch = svc->createCharacteristic(RL_RESPONSE_COUNT_CH, READ | NOTIFY);
	response_count_ch->addDescriptor(new BLE2902());

	firmware_version_ch = svc->createCharacteristic(RL_FIRMWARE_VERSION_CH, READ);
	firmware_version_ch->setValue(SUBG_RFSPY_VERSION);

#if 0
	ch = svc->createCharacteristic(RL_TIMER_TICK_CH, READ | NOTIFY);
	ch = svc->createCharacteristic(RL_CUSTOM_NAME_CH, READ | WRITE);
	ch = svc->createCharacteristic(RL_LED_MODE_CH, READ | WRITE);
#endif

	svc->start();

	BLEAdvertising* adv = srv->getAdvertising();
	adv->addServiceUUID(RL_SERVICE_UUID);
	adv->setScanResponse(false);
	adv->start();
}
