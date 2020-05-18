// Configuration for TTGO ESP32 LoRa OLED v1 module

#define BUTTON		GPIO_NUM_0
#define LED		GPIO_NUM_2

// Comment out the following line for a TTGO ESP LoRa v1 module w/o OLED
#define OLED_ENABLE
#define OLED_SDA	GPIO_NUM_4
#define OLED_SCL	GPIO_NUM_15
#define OLED_RST	GPIO_NUM_16

#define LORA_SCK	GPIO_NUM_5
#define LORA_MISO	GPIO_NUM_19
#define LORA_MOSI	GPIO_NUM_27
#define LORA_CS		GPIO_NUM_18
#define LORA_RST	GPIO_NUM_14
#define LORA_IRQ	GPIO_NUM_26

