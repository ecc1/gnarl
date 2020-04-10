idf_component_register(
	INCLUDE_DIRS .
	SRC_DIRS .
	EMBED_TXTFILES ../../include/root_cert.pem
	PRIV_REQUIRES esp_http_client json
)
