#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "medtronic.h"
#include "commands.h"
#include "pump_history.h"

// Decode a 5-byte timestamp from a pump history record.
time_t decode_time(uint8_t *data) {
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

// Return 1 for insulin-related events, 0 for events to skip, -1 on error.
static int decode_history_record(uint8_t *data, int len, int family, history_record_t *r) {
	memset(r, 0, sizeof(*r));
	r->type = data[0];
	r->length = 7;
	switch (r->type) {
	case Bolus:
		if (family <= 22) {
			r->length = 9;
			r->time = decode_time(&data[4]);
			r->insulin = int_to_insulin(data[2], family);
			r->duration = half_hours(data[3]);
		} else {
			r->length = 13;
			r->time = decode_time(&data[8]);
			r->insulin = int_to_insulin(two_byte_be_int(&data[3]), family);
			r->duration = half_hours(data[7]);
		}
		return 1;
	case Prime:
		r->length = 10;
		return 0;
	case Alarm:
		r->length = 9;
		return 0;
	case DailyTotal:
		r->length = family <= 22 ? 7 : 10;
		return 0;
	case BasalProfileBefore:
		r->length = 152;
		return 0;
	case BasalProfileAfter:
		r->length = 152;
		return 0;
	case BGCapture:
		return 0;
	case SensorAlarm:
		r->length = 8;
		return 0;
	case ClearAlarm:
		return 0;
	case ChangeBasalPattern:
		return 0;
	case TempBasalDuration:
		r->time = decode_time(&data[2]);
		r->duration = half_hours(data[1]);
		return 1;
	case ChangeTime:
		return 0;
	case NewTime:
		return 0;
	case LowBattery:
		return 0;
	case BatteryChange:
		return 0;
	case SetAutoOff:
		return 0;
	case PrepareInsulinChange:
		return 0;
	case SuspendPump:
		r->time = decode_time(&data[2]);
		return 1;
	case ResumePump:
		r->time = decode_time(&data[2]);
		return 1;
	case SelfTest:
		return 0;
	case Rewind:
		return 0;
	case ClearSettings:
		return 0;
	case EnableChildBlock:
		return 0;
	case MaxBolus:
		return 0;
	case EnableRemote:
		r->length = 21;
		return 0;
	case MaxBasal:
		return 0;
	case EnableBolusWizard:
		return 0;
	case Unknown2E:
		r->length = 107;
		return 0;
	case BolusWizard512:
		r->length = 19;
		return 0;
	case UnabsorbedInsulin512:
		r->length = data[1];
		return 0;
	case ChangeBGReminder:
		return 0;
	case SetAlarmClockTime:
		return 0;
	case TempBasalRate:
		r->length = 8;
		r->time = decode_time(&data[2]);
		switch (data[7] >> 3) { // temp basal type
		case ABSOLUTE:
			r->insulin = int_to_insulin(((data[7] & 0x7) << 8) | data[1], 23);
			return 1;
		default:
			ESP_LOGE(TAG, "ignoring %3d percent temp basal in pump history at %s", data[1], time_string(r->time));
			return 0;
		}
	case LowReservoir:
		return 0;
	case AlarmClock:
		return 0;
	case ChangeMeterID:
		r->length = 21;
		return 0;
	case BGReceived512:
		r->length = 10;
		return 0;
	case ConfirmInsulinChange:
		return 0;
	case SensorStatus:
		return 0;
	case EnableMeter:
		r->length = 21;
		return 0;
	case BGReceived:
		r->length = 10;
		return 0;
	case MealMarker:
		r->length = 9;
		return 0;
	case ExerciseMarker:
		r->length = 8;
		return 0;
	case InsulinMarker:
		r->length = 8;
		return 0;
	case OtherMarker:
		return 0;
	case EnableSensorAutoCal:
		return 0;
	case ChangeBolusWizardSetup:
		r->length = 39;
		return 0;
	case SensorSetup:
		r->length = family >= 51 ? 41 : 37;
		return 0;
	case Sensor51:
		return 0;
	case Sensor52:
		return 0;
	case ChangeSensorAlarm:
		r->length = 8;
		return 0;
	case Sensor54:
		r->length = 64;
		return 0;
	case Sensor55:
		r->length = 55;
		return 0;
	case ChangeSensorAlert:
		r->length = 12;
		return 0;
	case ChangeBolusStep:
		return 0;
	case BolusWizardSetup:
		r->length = family <= 22 ? 124 : 144;
		return 0;
	case BolusWizard:
		r->length = family <= 22 ? 20 : 22;
		return 0;
	case UnabsorbedInsulin:
		r->length = data[1];
		return 0;
	case SaveSettings:
		return 0;
	case EnableVariableBolus:
		return 0;
	case ChangeEasyBolus:
		return 0;
	case EnableBGReminder:
		return 0;
	case EnableAlarmClock:
		return 0;
	case ChangeTempBasalType:
		return 0;
	case ChangeAlarmType:
		return 0;
	case ChangeTimeFormat:
		return 0;
	case ChangeReservoirWarning:
		return 0;
	case EnableBolusReminder:
		return 0;
	case SetBolusReminderTime:
		r->length = 9;
		return 0;
	case DeleteBolusReminderTime:
		r->length = 9;
		return 0;
	case BolusReminder:
		r->length = 9;
		return 0;
	case DeleteAlarmClockTime:
		return 0;
	case DailyTotal515:
		r->length = 38;
		return 0;
	case DailyTotal522:
		r->length = 44;
		return 0;
	case DailyTotal523:
		r->length = 52;
		return 0;
	case ChangeCarbUnits:
		return 0;
	case BasalProfileStart:
		r->length = 10;
		r->time = decode_time(&data[2]);
		// data[7] = starting half-hour
		r->insulin = int_to_insulin(two_byte_le_int(&data[8]), 23);
		return 1;
	case ConnectOtherDevices:
		return 0;
	case ChangeOtherDevice:
		r->length = 37;
		return 0;
	case ChangeMarriage:
		r->length = 12;
		return 0;
	case DeleteOtherDevice:
		r->length = 12;
		return 0;
	case EnableCaptureEvent:
		return 0;
	default:
		return -1;
	}
}

static int all_zero(uint8_t *data, int len) {
	for (int i = 0; i < len; i++) {
		if (data[i] != 0) {
			return 0;
		}
	}
	return 1;
}

void print_bytes(const char *msg, const uint8_t *data, int len) {
	printf("%s:", msg);
	for (int i = 0; i < len; i++) {
		printf(" %02X", data[i]);
	}
	printf("\n");
}

int decode_history(uint8_t *page, int family, history_record_t *r, int max) {
	uint8_t *data = page;
	int len = HISTORY_PAGE_SIZE;
	int count = 0;
	while (count < max) {
		if (all_zero(data, len)) {
			break;
		}
		int e = decode_history_record(data, len, family, r);
		if (e == -1) {
			ESP_LOGE(TAG, "unknown history record type %02X", data[0]);
			print_bytes("history data", data, len);
			break;
		}
		data += r->length;
		len -= r->length;
		if (e == 1) {
			count++;
			r++;
		}
	}
	return count;
}
