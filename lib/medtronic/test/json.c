#include <jansson.h>

#include "testing.h"

static json_t *json_object_path(json_t *obj, const char *path) {
	char buf[20];
	for (;;) {
		char *p = strchr(path, '.');
		if (p == 0) {
			return json_object_get(obj, path);
		}
		int n = p - path;
		strncpy(buf, path, n);
		buf[n] = 0;
		obj = json_object_get(obj, buf);
		path = p + 1;
	}
}

static time_t json_time_value(json_t *t) {
	return parse_json_time(json_string_value(t));
}

static time_t json_duration_value(json_t *t) {
	return parse_duration(json_string_value(t));
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
static uint8_t json_data0_value(json_t *d) {
	const char *s = json_string_value(d);
	uint8_t a = s[0], b = s[1];
	return (decode_table[a] << 2) | (decode_table[b] >> 4);
}

// Find record with matching timestamp and type.
static history_record_t *find_object(json_t *obj) {
	json_t *jt = json_object_get(obj, "Time");
	if (jt == 0) {
		// All records of interest have time stamps.
		return 0;
	}
	time_t t = json_time_value(jt);
	history_record_type_t type = json_data0_value(json_object_get(obj, "Data"));
	for (int i = 0; i < history_length; i++) {
		history_record_t *r = &history[i];
		if (r->time == t && r->type == type) {
			return r;
		}
	}
	return 0;
}

static void check_insulin(history_record_t *r, json_t *obj) {
	double v = json_number_value(obj);
	if (r->insulin != (int)(1000 * v)) {
		double u = r->insulin / 1000.0;
		test_failed("[%s] %s insulin = %g, JSON value = %g", time_string(r->time), type_string(r->type), u, v);
	}
}

static void check_duration(history_record_t *r, json_t *obj) {
	time_t d = json_duration_value(obj);
	if (r->duration != d) {
		test_failed("[%s] %s duration = %d, JSON value = %d", time_string(r->time), type_string(r->type), r->duration / 60, d / 60);
	}
}

static int records_matched;

static void check_object(json_t *obj) {
	history_record_t *r = find_object(obj);
	if (r == 0) {
		return;
	}
	switch (r->type) {
	case Bolus:
		check_insulin(r, json_object_path(obj, "Info.Amount"));
		check_duration(r, json_object_path(obj, "Info.Duration"));
		break;
	case TempBasalDuration:
		check_duration(r, json_object_get(obj, "Info"));
		break;
	case SuspendPump:
		break;
	case ResumePump:
		break;
	case TempBasalRate:
		check_insulin(r, json_object_path(obj, "Info.Value"));
		break;
	case BasalProfileStart:
		check_insulin(r, json_object_path(obj, "Info.BasalRate.Rate"));
		break;
	default:
		fprintf(stderr, "unexpected %s record at %s\n", type_string(r->type), time_string(r->time));
		exit(1);
	}
	// Zero out record so it is not considered again.
	memset(r, 0, sizeof(*r));
	records_matched++;
}

void compare_with_json(char *filename) {
	FILE *f = fopen(filename, "r");
	if (f == 0) {
		perror(filename);
		exit(1);
	}
	json_error_t error;
	json_t *root = json_loadf(f, 0, &error);
	fclose(f);
	if (root == 0) {
		fprintf(stderr, "%s:%d: %s\n", filename, error.line, error.text);
		exit(1);
	}
	if (!json_is_array(root)) {
		fprintf(stderr, "%s: root is not a JSON array\n", filename);
		exit(1);
	}
	records_matched = 0;
	for (int i = 0; i < json_array_size(root); i++) {
		json_t *obj = json_array_get(root, i);
		if (!json_is_object(obj)) {
			fprintf(stderr, "%s: element %d is not a JSON object\n", filename, i);
			exit(1);
		}
		check_object(obj);
	}
	json_decref(root);
	if (records_matched != history_length) {
		test_failed("[%s] matched %d JSON records out of %d", filename, records_matched, history_length);
	}
}
