#include <stdio.h>

#include "medtronic.h"
#include "commands.h"
#include "pump_history.h"

// Decode a 5-byte timestamp from a pump history record.
time_t pump_decode_time(uint8_t *data) {
	struct tm tm = {
		.tm_sec = data[0] & 0x3F,
		.tm_min = data[1] & 0x3F,
		.tm_hour = data[2] & 0x1F,
		.tm_mday = data[3] & 0x1F,
		// The 4-bit month value is encoded in the high 2 bits of the first 2 bytes.
		.tm_mon = (((data[0]>>6)<<2) | (data[1]>>6)) - 1,
		.tm_year = (data[4]&0x7F) + 100,
		.tm_isdst = -1,
	};
	return mktime(&tm);
}

#define UNKNOWN_RECORD_ERR	(-1)
#define RECORD_SIZE_ERR		(-2)

#define REQUIRE_BYTES(n)					\
	do {							\
		r->length = (n);				\
		if (r->length > len) return RECORD_SIZE_ERR;	\
	} while (0)

// Return 1 for insulin-related records, 0 for records to skip, negative values for errors.
// Insulin-related records:
//   BasalProfileStart (x23 and newer models only)
//   TempBasalRate and TempBasalDuration
//   SuspendPump and ResumePump
//   Bolus (normal and square wave)
//   Rewind and Prime
//   Alarm and ClearAlarm
static int decode_history_record(uint8_t *data, int len, int family, history_record_t *r) {
	memset(r, 0, sizeof(history_record_t));
	r->type = data[0];
	switch (r->type) {
	case Bolus:
		if (family <= 22) {
			REQUIRE_BYTES(9);
			r->time = pump_decode_time(&data[4]);
			r->insulin = int_to_insulin(data[2], family);
			r->duration = half_hours(data[3]);
		} else {
			REQUIRE_BYTES(13);
			r->time = pump_decode_time(&data[8]);
			r->insulin = int_to_insulin(two_byte_be_int(&data[3]), family);
			r->duration = half_hours(data[7]);
		}
		return 1;
	case Prime:
		REQUIRE_BYTES(10);
		r->time = pump_decode_time(&data[5]);
		return 1;
	case Alarm:
		REQUIRE_BYTES(9);
		r->time = pump_decode_time(&data[4]);
		// Use insulin field to store alarm code.
		r->insulin = data[1];
		return 1;
	case DailyTotal:
		REQUIRE_BYTES(family <= 22 ? 7 : 10);
		return 0;
	case BasalProfileBefore:
		REQUIRE_BYTES(152);
		return 0;
	case BasalProfileAfter:
		REQUIRE_BYTES(152);
		return 0;
	case BGCapture:
		REQUIRE_BYTES(7);
		return 0;
	case SensorAlarm:
		REQUIRE_BYTES(8);
		return 0;
	case ClearAlarm:
		REQUIRE_BYTES(7);
		r->time = pump_decode_time(&data[2]);
		return 1;
	case ChangeBasalPattern:
		REQUIRE_BYTES(7);
		return 0;
	case TempBasalDuration:
		REQUIRE_BYTES(7);
		r->time = pump_decode_time(&data[2]);
		r->duration = half_hours(data[1]);
		return 1;
	case ChangeTime:
		REQUIRE_BYTES(7);
		return 0;
	case NewTime:
		REQUIRE_BYTES(7);
		return 0;
	case LowBattery:
		REQUIRE_BYTES(7);
		return 0;
	case BatteryChange:
		REQUIRE_BYTES(7);
		return 0;
	case SetAutoOff:
		REQUIRE_BYTES(7);
		return 0;
	case PrepareInsulinChange:
		REQUIRE_BYTES(7);
		return 0;
	case SuspendPump:
		REQUIRE_BYTES(7);
		r->time = pump_decode_time(&data[2]);
		return 1;
	case ResumePump:
		REQUIRE_BYTES(7);
		r->time = pump_decode_time(&data[2]);
		return 1;
	case SelfTest:
		REQUIRE_BYTES(7);
		return 0;
	case Rewind:
		REQUIRE_BYTES(7);
		r->time = pump_decode_time(&data[2]);
		return 1;
	case ClearSettings:
		REQUIRE_BYTES(7);
		return 0;
	case EnableChildBlock:
		REQUIRE_BYTES(7);
		return 0;
	case MaxBolus:
		REQUIRE_BYTES(7);
		return 0;
	case EnableRemote:
		REQUIRE_BYTES(21);
		return 0;
	case MaxBasal:
		REQUIRE_BYTES(7);
		return 0;
	case EnableBolusWizard:
		REQUIRE_BYTES(7);
		return 0;
	case Unknown2E:
		REQUIRE_BYTES(107);
		return 0;
	case BolusWizard512:
		REQUIRE_BYTES(19);
		return 0;
	case UnabsorbedInsulin512:
		REQUIRE_BYTES(data[1]);
		return 0;
	case ChangeBGReminder:
		REQUIRE_BYTES(7);
		return 0;
	case SetAlarmClockTime:
		REQUIRE_BYTES(7);
		return 0;
	case TempBasalRate:
		REQUIRE_BYTES(8);
		r->time = pump_decode_time(&data[2]);
		switch (data[7] >> 3) { // temp basal type
		case ABSOLUTE:
			r->insulin = int_to_insulin(((data[7] & 0x7) << 8) | data[1], 23);
			return 1;
		default:
			if (data[1] == 0) {
				r->insulin = 0;
				return 1;
			}
			char ts[TIME_STRING_SIZE];
			ESP_LOGE(TAG, "%3d percent temp basal in pump history at %s",
				 data[1], time_string(r->time, ts));
			return 0;
		}
	case LowReservoir:
		REQUIRE_BYTES(7);
		return 0;
	case AlarmClock:
		REQUIRE_BYTES(7);
		return 0;
	case ChangeMeterID:
		REQUIRE_BYTES(21);
		return 0;
	case BGReceived512:
		REQUIRE_BYTES(10);
		return 0;
	case ConfirmInsulinChange:
		REQUIRE_BYTES(7);
		return 0;
	case SensorStatus:
		REQUIRE_BYTES(7);
		return 0;
	case EnableMeter:
		REQUIRE_BYTES(21);
		return 0;
	case BGReceived:
		REQUIRE_BYTES(10);
		return 0;
	case MealMarker:
		REQUIRE_BYTES(9);
		return 0;
	case ExerciseMarker:
		REQUIRE_BYTES(8);
		return 0;
	case InsulinMarker:
		REQUIRE_BYTES(8);
		return 0;
	case OtherMarker:
		REQUIRE_BYTES(7);
		return 0;
	case EnableSensorAutoCal:
		REQUIRE_BYTES(7);
		return 0;
	case ChangeBolusWizardSetup:
		REQUIRE_BYTES(39);
		return 0;
	case SensorSetup:
		REQUIRE_BYTES(family >= 51 ? 41 : 37);
		return 0;
	case Sensor51:
		REQUIRE_BYTES(7);
		return 0;
	case Sensor52:
		REQUIRE_BYTES(7);
		return 0;
	case ChangeSensorAlarm:
		REQUIRE_BYTES(8);
		return 0;
	case Sensor54:
		REQUIRE_BYTES(64);
		return 0;
	case Sensor55:
		REQUIRE_BYTES(55);
		return 0;
	case ChangeSensorAlert:
		REQUIRE_BYTES(12);
		return 0;
	case ChangeBolusStep:
		REQUIRE_BYTES(7);
		return 0;
	case BolusWizardSetup:
		REQUIRE_BYTES(family <= 22 ? 124 : 144);
		return 0;
	case BolusWizard:
		REQUIRE_BYTES(family <= 22 ? 20 : 22);
		return 0;
	case UnabsorbedInsulin:
		REQUIRE_BYTES(data[1]);
		return 0;
	case SaveSettings:
		REQUIRE_BYTES(7);
		return 0;
	case EnableVariableBolus:
		REQUIRE_BYTES(7);
		return 0;
	case ChangeEasyBolus:
		REQUIRE_BYTES(7);
		return 0;
	case EnableBGReminder:
		REQUIRE_BYTES(7);
		return 0;
	case EnableAlarmClock:
		REQUIRE_BYTES(7);
		return 0;
	case ChangeTempBasalType:
		REQUIRE_BYTES(7);
		return 0;
	case ChangeAlarmType:
		REQUIRE_BYTES(7);
		return 0;
	case ChangeTimeFormat:
		REQUIRE_BYTES(7);
		return 0;
	case ChangeReservoirWarning:
		REQUIRE_BYTES(7);
		return 0;
	case EnableBolusReminder:
		REQUIRE_BYTES(7);
		return 0;
	case SetBolusReminderTime:
		REQUIRE_BYTES(9);
		return 0;
	case DeleteBolusReminderTime:
		REQUIRE_BYTES(9);
		return 0;
	case BolusReminder:
		REQUIRE_BYTES(9);
		return 0;
	case DeleteAlarmClockTime:
		REQUIRE_BYTES(7);
		return 0;
	case DailyTotal515:
		REQUIRE_BYTES(38);
		return 0;
	case DailyTotal522:
		REQUIRE_BYTES(44);
		return 0;
	case DailyTotal523:
		REQUIRE_BYTES(52);
		return 0;
	case ChangeCarbUnits:
		REQUIRE_BYTES(7);
		return 0;
	case BasalProfileStart:
		REQUIRE_BYTES(10);
		r->time = pump_decode_time(&data[2]);
		// data[7] = starting half-hour
		r->insulin = int_to_insulin(two_byte_le_int(&data[8]), 23);
		return 1;
	case ConnectOtherDevices:
		REQUIRE_BYTES(7);
		return 0;
	case ChangeOtherDevice:
		REQUIRE_BYTES(37);
		return 0;
	case ChangeMarriage:
		REQUIRE_BYTES(12);
		return 0;
	case DeleteOtherDevice:
		REQUIRE_BYTES(12);
		return 0;
	case EnableCaptureEvent:
		REQUIRE_BYTES(7);
		return 0;
	default:
		return UNKNOWN_RECORD_ERR;
	}
}

static bool all_zero(uint8_t *data, int len) {
	for (int i = 0; i < len; i++) {
		if (data[i] != 0) {
			return false;
		}
	}
	return true;
}

void print_bytes(const char *msg, const uint8_t *data, int len) {
	printf("%s:", msg);
	for (int i = 0; i < len; i++) {
		printf(" %02X", data[i]);
	}
	printf("\n");
}

void pump_decode_history(uint8_t *page, int len, int family, history_record_fn_t decode_fn) {
	uint8_t *data = page;
	history_record_t rec;
	while (len > 0) {
		if (all_zero(data, len)) {
			return;
		}
		int e = decode_history_record(data, len, family, &rec);
		switch (e) {
		case UNKNOWN_RECORD_ERR:
			ESP_LOGE(TAG, "unknown history record type %02X", rec.type);
			print_bytes("history data", data, len);
			return;
		case RECORD_SIZE_ERR:
			ESP_LOGE(TAG, "history record type %02X would require %d bytes", rec.type, rec.length);
			print_bytes("history data", data, len);
			return;
		default:
			if (e < 0) {
				ESP_LOGE(TAG, "history record type %02X: unknown error %d", rec.type, e);
				return;
			}
			break;
		}
		if (e == 1 && decode_fn(&rec) != 0) {
			return;
		}
		data += rec.length;
		len -= rec.length;
	}
}
