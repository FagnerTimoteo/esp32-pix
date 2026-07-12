set(component_srcs
	"fontx.c"
	"lcd_com.c"
	"lcd_lib.c"
	"st7796.c"
)

idf_component_register(SRCS "${component_srcs}"
                       PRIV_REQUIRES esp_lcd
                       PRIV_REQUIRES esp_driver_spi
                       INCLUDE_DIRS ".")
