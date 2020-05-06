#ifndef _PUMP_HISTORY_H
#define _PUMP_HISTORY_H

// These definitions track github.com/ecc1/medtronic/historyrecord.go
typedef enum PACKED {
	Bolus                   = 0x01,
	Prime                   = 0x03,
	Alarm                   = 0x06,
	DailyTotal              = 0x07,
	BasalProfileBefore      = 0x08,
	BasalProfileAfter       = 0x09,
	BGCapture               = 0x0A,
	SensorAlarm             = 0x0B,
	ClearAlarm              = 0x0C,
	ChangeBasalPattern      = 0x14,
	TempBasalDuration       = 0x16,
	ChangeTime              = 0x17,
	NewTime                 = 0x18,
	LowBattery              = 0x19,
	BatteryChange           = 0x1A,
	SetAutoOff              = 0x1B,
	PrepareInsulinChange    = 0x1C,
	SuspendPump             = 0x1E,
	ResumePump              = 0x1F,
	SelfTest                = 0x20,
	Rewind                  = 0x21,
	ClearSettings           = 0x22,
	EnableChildBlock        = 0x23,
	MaxBolus                = 0x24,
	EnableRemote            = 0x26,
	MaxBasal                = 0x2C,
	EnableBolusWizard       = 0x2D,
	Unknown2E               = 0x2E,
	BolusWizard512          = 0x2F,
	UnabsorbedInsulin512    = 0x30,
	ChangeBGReminder        = 0x31,
	SetAlarmClockTime       = 0x32,
	TempBasalRate           = 0x33,
	LowReservoir            = 0x34,
	AlarmClock              = 0x35,
	ChangeMeterID           = 0x36,
	BGReceived512           = 0x39,
	ConfirmInsulinChange    = 0x3A,
	SensorStatus            = 0x3B,
	EnableMeter             = 0x3C,
	BGReceived              = 0x3F,
	MealMarker              = 0x40,
	ExerciseMarker          = 0x41,
	InsulinMarker           = 0x42,
	OtherMarker             = 0x43,
	EnableSensorAutoCal     = 0x44,
	ChangeBolusWizardSetup  = 0x4F,
	SensorSetup             = 0x50,
	Sensor51                = 0x51,
	Sensor52                = 0x52,
	ChangeSensorAlarm       = 0x53,
	Sensor54                = 0x54,
	Sensor55                = 0x55,
	ChangeSensorAlert       = 0x56,
	ChangeBolusStep         = 0x57,
	BolusWizardSetup        = 0x5A,
	BolusWizard             = 0x5B,
	UnabsorbedInsulin       = 0x5C,
	SaveSettings            = 0x5D,
	EnableVariableBolus     = 0x5E,
	ChangeEasyBolus         = 0x5F,
	EnableBGReminder        = 0x60,
	EnableAlarmClock        = 0x61,
	ChangeTempBasalType     = 0x62,
	ChangeAlarmType         = 0x63,
	ChangeTimeFormat        = 0x64,
	ChangeReservoirWarning  = 0x65,
	EnableBolusReminder     = 0x66,
	SetBolusReminderTime    = 0x67,
	DeleteBolusReminderTime = 0x68,
	BolusReminder           = 0x69,
	DeleteAlarmClockTime    = 0x6A,
	DailyTotal515           = 0x6C,
	DailyTotal522           = 0x6D,
	DailyTotal523           = 0x6E,
	ChangeCarbUnits         = 0x6F,
	BasalProfileStart       = 0x7B,
	ConnectOtherDevices     = 0x7C,
	ChangeOtherDevice       = 0x7D,
	ChangeMarriage          = 0x81,
	DeleteOtherDevice       = 0x82,
	EnableCaptureEvent      = 0x83,
} history_record_type_t;

typedef struct {
	history_record_type_t type;
	uint8_t length;
	time_t time;
	insulin_t insulin;
	int duration;		// seconds
} history_record_t;

typedef enum PACKED {
	BatteryOutLimitExceeded = 0x03,
	NoDelivery              = 0x04,
	BatteryDepleted         = 0x05,
	AutoOff                 = 0x06,
	DeviceReset             = 0x10,
	ReprogramError          = 0x3D,
	EmptyReservoir          = 0x3E,
} alarm_code_t;

time_t pump_decode_time(uint8_t *data);

// Signature of function to be applied to history records during decoding.
typedef int (*history_record_fn_t)(history_record_t *);

// Decode the given history page and apply f to each insulin-related record.
// If f returns a non-zero value, the decoding loop terminates.
void pump_decode_history(uint8_t *page, int len, int family, history_record_fn_t decode_fn);

#endif // _PUMP_HISTORY_H
