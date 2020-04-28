#include <cJSON.h>

#include "medtronic_test.h"

static cJSON *object_path(cJSON *obj, const char *path) {
	char buf[20];
	for (;;) {
		char *p = strchr(path, '.');
		if (p == 0) {
			return cJSON_GetObjectItem(obj, path);
		}
		int n = p - path;
		strncpy(buf, path, n);
		buf[n] = 0;
		obj = cJSON_GetObjectItem(obj, buf);
		path = p + 1;
	}
}

static time_t json_time_value(cJSON *t) {
	return parse_json_time(t->valuestring);
}

static time_t json_duration_value(cJSON *t) {
	return parse_duration(t->valuestring);
}

static const uint8_t decode_table[256] = {
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
	64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
	64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
};

// Decode the first byte of a base64-encoded string.
static uint8_t json_data0_value(cJSON *d) {
	const char *s = d->valuestring;
	uint8_t a = s[0], b = s[1];
	return (decode_table[a] << 2) | (decode_table[b] >> 4);
}

// Find record with matching timestamp and type.
static history_record_t *find_object(cJSON *obj) {
	cJSON *jt = cJSON_GetObjectItem(obj, "Time");
	if (!jt) {
		// All records of interest have time stamps.
		return 0;
	}
	time_t t = json_time_value(jt);
	history_record_type_t type = json_data0_value(cJSON_GetObjectItem(obj, "Data"));
	for (int i = 0; i < history_length; i++) {
		history_record_t *r = &history[i];
		if (r->time == t && r->type == type) {
			return r;
		}
	}
	return 0;
}

static void check_insulin(history_record_t *r, cJSON *obj) {
	double v = obj->valuedouble;
	if (r->insulin != (insulin_t)(1000 * v)) {
		double u = (double)r->insulin / 1000;
		char ts[TIME_STRING_SIZE];
		test_failed("[%s] %s insulin = %g, JSON value = %g", time_string(r->time, ts), type_string(r->type), u, v);
	}
}

static void check_duration(history_record_t *r, cJSON *obj) {
	time_t d = json_duration_value(obj);
	if (r->duration != d) {
		char ts[TIME_STRING_SIZE];
		test_failed("[%s] %s duration = %d, JSON value = %d", time_string(r->time, ts), type_string(r->type), r->duration / 60, d / 60);
	}
}

static int records_matched;

// This check must cover all record types for which decode_history_record can return 1.
static void check_object(cJSON *obj) {
	char ts[TIME_STRING_SIZE];
	history_record_t *r = find_object(obj);
	if (r == 0) {
		return;
	}
	switch (r->type) {
	case Bolus:
		check_insulin(r, object_path(obj, "Info.Amount"));
		check_duration(r, object_path(obj, "Info.Duration"));
		break;
	case Prime:
		break;
	case Alarm:
		break;
	case ClearAlarm:
		break;
	case TempBasalDuration:
		check_duration(r, cJSON_GetObjectItem(obj, "Info"));
		break;
	case SuspendPump:
		break;
	case ResumePump:
		break;
	case Rewind:
		break;
	case TempBasalRate:
		check_insulin(r, object_path(obj, "Info.Value"));
		break;
	case BasalProfileStart:
		check_insulin(r, object_path(obj, "Info.BasalRate.Rate"));
		break;
	default:
		fprintf(stderr, "unexpected %s record at %s\n", type_string(r->type), time_string(r->time, ts));
		exit(1);
	}
	// Zero out record so it is not considered again.
	memset(r, 0, sizeof(*r));
	records_matched++;
}

void compare_with_json(char *filename) {
	char *json_string = read_file(filename);
	cJSON *root = cJSON_Parse(json_string);
	assert(root != 0);
	assert(cJSON_IsArray(root));
	int n = cJSON_GetArraySize(root);
	records_matched = 0;
	for (int i = 0; i < n; i++) {
		check_object(cJSON_GetArrayItem(root, i));
	}
	cJSON_Delete(root);
	if (records_matched != history_length) {
		test_failed("[%s] matched %d JSON records out of %d", filename, records_matched, history_length);
	}
}
