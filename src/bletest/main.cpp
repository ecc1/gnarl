#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2904.h>
#include <esp_timer.h>

#define SERVICE_UUID	"f6b35e8c-b4a7-4838-8569-e9091b9a8b0e"
#define TIMER_CH	"6952b350-8d61-4461-b9c9-b1ebfa70a838"

static void update(BLECharacteristic *ch) {
	uint32_t ms = esp_timer_get_time() / 1000;
	printf("updating value to %d\n", ms);
	ch->setValue(ms);
}

class ch_callbacks: public BLECharacteristicCallbacks {
	void onRead(BLECharacteristic *ch) {
		update(ch);
	}
};

class server_callbacks: public BLEServerCallbacks {
	void onConnect(BLEServer* srv) {
		printf("connected\n");
	}

	void onDisconnect(BLEServer* srv) {
		printf("disconnected\n");
	}
};

extern "C" void app_main() {
	BLEDevice::init("ESP32");

	BLEServer* srv = BLEDevice::createServer();
	srv->setCallbacks(new server_callbacks);
	BLEService* svc = srv->createService(SERVICE_UUID);

	BLECharacteristic* ch = svc->createCharacteristic(TIMER_CH, BLECharacteristic::PROPERTY_READ);
	BLE2904 *desc = new BLE2904();
	desc->setFormat(BLE2904::FORMAT_UINT32);
	ch->addDescriptor(desc);
	ch->setCallbacks(new ch_callbacks);
	update(ch);

	svc->start();

	BLEAdvertising* adv = srv->getAdvertising();
	adv->addServiceUUID(SERVICE_UUID);
	adv->setScanResponse(false);
	adv->start();
}
