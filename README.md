# Esp32-pix

Integração entre ESP32, gateway PIX e display TFT SPI ST7796S 3,5" 320x480.

Este projeto usa o framework [ESP-IDF](https://github.com/espressif/esp-idf).

## Hardware Atual

O trabalho base foi ajustado para usar um display ST7796S SPI de 3,5 polegadas, resolução 320x480. A tela inicial de autoteste foi removida; o firmware já entra direto no fluxo da aplicação.

O QR Code é desenhado centralizado para 320x480, o timer fica no rodapé e as limpezas de tela foram ajustadas para evitar resíduos de imagens anteriores.

## Pinagem

### Display ST7796S SPI

| Display | ESP32 | Observação |
|---|---:|---|
| GND | GND | Terra comum |
| VCC | 3.3V | Use 3.3V conforme o módulo em uso |
| SCL / SCK / CLK | GPIO18 | Clock SPI |
| SDA / SDI / MOSI | GPIO23 | Dados SPI para o display |
| RST / RESET | GPIO4 | Reset do display |
| DC / RS | GPIO2 | Data/Command |
| CS | GPIO5 | Chip Select |
| BL / LED | 3.3V | Backlight sempre ligado |
| SDA-0 / SDO / MISO | Não ligar | O firmware não usa leitura do display |

No driver, esses pinos estão definidos em `components/tft_library/st7796.c`.

### Botão

O botão de pagamento usa o GPIO35:

| Botão | ESP32 |
|---|---:|
| Terminal 1 | GPIO35 |
| Terminal 2 | GND |
| Pull-up externo | 10k entre GPIO35 e 3.3V |

O GPIO35 é somente entrada e não possui pull-up interno útil para este caso. O código considera o botão pressionado quando o pino lê nível baixo.

### Atuador

| Atuador | ESP32 |
|---|---:|
| Sinal | GPIO15 |

O GPIO15 é acionado quando o pagamento é confirmado. Use driver/transistor/relé adequado se a carga consumir mais corrente do que o ESP32 suporta.

## Configuração

Antes de compilar, carregue o ambiente ESP-IDF e configure o alvo:

```powershell
& 'C:\Espressif\frameworks\esp-idf-v5.3.1\export.ps1'
idf.py set-target esp32
idf.py menuconfig
```

No menu **PIX Configuration**, ajuste:

1. `PIX_GATEWAY_HOST`: _hostname_ do _gateway_;
2. `PIX_GATEWAY_HOST`: Define se as chamadas ao _gateway_ usarão ou não HTTPS;

No menu **WIFI Configuration**, ajuste:

1. `LOAD_FROM_SD_CARD`: define se a configuração do Wi-Fi vem do cartão SD ou do menuconfig.
2. `ESP_WIFI_SSID`: nome da rede Wi-Fi.
3. `ESP_WIFI_PASSWORD`: senha da rede Wi-Fi.

Caso a configuração venha do cartão SD, coloque um arquivo `config.txt` na raiz do cartão com duas linhas:

```text
SSID
senha
```

No menu **SD CARD SPI Configuration** ajuste a pinagem de conexão com o SD Card.
No menu **TFT Configuration** escolha o driver e a pinagem do display LCD.

## Compilação

Na raiz do projeto:

```powershell
idf.py build
```

Para gravar no ESP32, ajuste a porta se necessário:

```powershell
idf.py -p COM9 flash
```

Se a porta estiver ocupada, feche o monitor serial ou outro programa usando a COM.

## Módulos Principais

1. `main`: fluxo principal da aplicação, botão, estados de pagamento, QR Code e acionamento do atuador.
2. `tft_library`: comunicação com o display ST7796S SPI.
3. `qrcodegen`: biblioteca usada para gerar o QR Code. Desenvolvida por nayuki, https://github.com/nayuki/QR-Code-generator/tree/master/c;
4. `http_client`: chamadas HTTP/HTTPS ao gateway PIX.
5. `wifi_station`: configuração e conexão Wi-Fi.
6. `sdcard`: leitura opcional de configuração via cartão SD.

## Funcionamento

Um vídeo do sistema em funcionamento pode ser visto em [https://youtu.be/LkkqwxMjYC8](https://youtu.be/LkkqwxMjYC8)

O firmware conecta ao Wi-Fi, exibe a última compra, aguarda o botão, solicita o QR Code ao gateway PIX, desenha o QR Code no display e consulta periodicamente o status do pagamento. Quando o pagamento é confirmado, o GPIO15 é acionado por alguns segundos.
