set(component_srcs "wifi_station.c")

idf_component_register(SRCS "${component_srcs}" 
	INCLUDE_DIRS "."
	PRIV_REQUIRES nvs_flash
	PRIV_REQUIRES esp_wifi
	PRIV_REQUIRES esp_event
	PRIV_REQUIRES esp_netif
)
