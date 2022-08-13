# xlib_2_ESP32_over_UDP
interogating the XServer and converting image to black and white before sending it to a SSD1306 over UDP.
This makes it posible to send a "video" live to the Adafruit SSD1306 128x64px black&white OLED Display


compile with:
  - gcc -o xlib_sender xlib_sender.c -lX11 -lm
  - ./xlib_sender 192.168.10.10  - your ESP32 IP Address

The Arduino ESP32 uses the WifiManager for Wifi provisioning and the UDPAsync Server
