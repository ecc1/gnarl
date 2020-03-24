// Pin mappings on TTGO ESP32 LoRa OLED v1 module

#define DIO0			GPIO_NUM_26
#define DIO1			GPIO_NUM_33
#define DIO2			GPIO_NUM_32

#define FXOSC			(32*MEGAHERTZ)

#define FIFO_SIZE		64

#define REG_FIFO		0x00
#define REG_OP_MODE		0x01
#define REG_BITRATE_MSB		0x02
#define REG_BITRATE_LSB		0x03
#define REG_FRF_MSB		0x06
#define REG_FRF_MID		0x07
#define REG_FRF_LSB		0x08
#define REG_LNA			0x0C
#define REG_RX_CONFIG		0x0D
#define REG_RSSI_CONFIG		0x0E
#define REG_RSSI		0x11
#define REG_RX_BW		0x12
#define REG_OOK_PEAK		0x14
#define REG_OOK_FIX		0x15
#define REG_OOK_AVG		0x16
#define REG_FEI_MSB		0x1D
#define REG_FEI_LSB		0x1E
#define REG_PREAMBLE_DETECT	0x1F
#define REG_PREAMBLE_MSB	0x25
#define REG_PREAMBLE_LSB	0x26
#define REG_SYNC_CONFIG		0x27
#define REG_SYNC_VALUE_1	0x28
#define REG_SYNC_VALUE_2	0x29
#define REG_SYNC_VALUE_3	0x2A
#define REG_SYNC_VALUE_4	0x2B
#define REG_PACKET_CONFIG_1	0x30
#define REG_PACKET_CONFIG_2	0x31
#define REG_PAYLOAD_LENGTH	0x32
#define REG_FIFO_THRESH		0x35
#define REG_SEQ_CONFIG_1	0x36
#define REG_IRQ_FLAGS_1		0x3E
#define REG_IRQ_FLAGS_2		0x3F
#define REG_DIO_MAPPING_1	0x40
#define REG_VERSION		0x42

// REG_OP_MODE
#define FSK_OOK_MODE		(0 << 7)
#define MODULATION_FSK		(0 << 5)
#define MODULATION_OOK		(1 << 5)

#define OP_MODE_MASK		0x7
#define MODE_SLEEP		0
#define MODE_STDBY		1
#define MODE_FSTX		2
#define MODE_TX			3
#define MODE_FSRX		4
#define MODE_RX			5

// REG_LNA
#define LNA_GAIN_MAX		(1 << 5)
#define LNA_GAIN_MAX_6DB	(2 << 5)
#define LNA_GAIN_MAX_12DB	(3 << 5)
#define LNA_GAIN_MAX_24DB	(4 << 5)
#define LNA_GAIN_MAX_36DB	(5 << 5)
#define LNA_GAIN_MAX_48DB	(6 << 5)

// REG_RX_CONFIG
#define AFC_AUTO_ON		(1 << 4)
#define AGC_AUTO_ON		(1 << 3)
#define RX_TRIGGER_PREAMBLE	(6 << 0)
#define RX_TRIGGER_RSSI		(1 << 0)

// REG_RX_BW
#define RX_BW_MANT_SHIFT	3

// REG_OOK_AVG
#define OOK_AVG_OFFSET_0DB	(0 << 2)
#define OOK_AVG_OFFSET_2DB	(1 << 2)
#define OOK_AVG_OFFSET_4DB	(2 << 2)
#define OOK_AVG_OFFSET_6DB	(3 << 2)

// REG_PREAMBLE_DETECT
#define PREAMBLE_DETECT_ON	(1 << 7)
#define PREAMBLE_3_BYTES	(2 << 5)

// REG_SYNC_CONFIG
#define SYNC_ON			(1 << 4)

// REG_PACKET_CONFIG_1
#define PACKET_FORMAT_FIXED	(0 << 7)
#define PACKET_FORMAT_VARIABLE	(1 << 7)

// REG_PACKET_CONFIG_2
#define PACKET_MODE		(1 << 6)
#define PAYLOAD_LENGTH_MSB_MASK	0x7

// REG_FIFO_THRESH
#define TX_START_CONDITION	(1 << 7)

// REG_SEQ_CONFIG_1
#define SEQUENCER_START			(1 << 7)
#define SEQUENCER_STOP			(1 << 6)
#define IDLE_MODE_STANDBY		(0 << 5)
#define IDLE_MODE_SLEEP			(1 << 5)
#define FROM_START_TO_LOW_POWER		(0 << 3)
#define FROM_START_TO_RX		(1 << 3)
#define FROM_START_TO_TX		(2 << 3)
#define FROM_START_TO_TX_ON_FIFO_LEVEL	(3 << 3)
#define	LOW_POWER_SELECT_OFF		(0 << 2)
#define	LOW_POWER_SELECT_IDLE		(1 << 2)
#define FROM_IDLE_TO_TX			(0 << 1)
#define FROM_IDLE_TO_RX			(1 << 1)
#define FROM_TX_TO_LOW_POWER		(0 << 0)
#define FROM_TX_TO_RX			(1 << 0)

// REG_IRQ_FLAGS_1
#define MODE_READY		(1 << 7)
#define RX_READY		(1 << 6)
#define TX_READY		(1 << 5)
#define SYNC_ADDRESS_MATCH	(1 << 0)

// REG_IRQ_FLAGS_2
#define FIFO_FULL		(1 << 7)
#define FIFO_EMPTY		(1 << 6)
#define FIFO_LEVEL		(1 << 5)
#define FIFO_OVERRUN		(1 << 4)

// REG_DIO_MAPPING_1
#define DIO0_MAPPING_SHIFT	6
#define DIO1_MAPPING_SHIFT	4
#define DIO2_MAPPING_SHIFT	2


void rfm95_init(void);

int read_version(void);

int version_major(int v);

int version_minor(int v);

uint32_t read_frequency(void);

void set_frequency(uint32_t freq_hz);

void transmit(uint8_t *buf, int count);

int tx_packet_count(void);

int receive(uint8_t *buf, int count, int timeout);

int sleep_receive(uint8_t *buf, int count, int timeout);

int read_rssi(void);

int rx_packet_count(void);
