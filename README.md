\# ESP32 Touchscreen-Controlled RGB LED Mirror



This project uses \*\*two ESP32 microcontrollers\*\*, a \*\*SPI capacitive touch TFT screen\*\*, and an \*\*RGB LED strip\*\* to create a dynamic light display that mirrors touch input from the screen. The LED strip is physically arranged to match the screen's dimensions (rectangle), so each touch on the screen corresponds to a specific LED on the strip.



---



\## ✨ Project Overview



\- \*\*ESP32 #1\*\* is connected to the \*\*TFT touchscreen display\*\*. It detects touch input and calculates the coordinates.

\- \*\*ESP32 #2\*\* is connected to the \*\*RGB LED strip\*\*. It lights up the LED that corresponds to the position touched on the screen.

\- The two ESP32s communicate wirelessly using \*\*ESP-NOW\*\*, a fast, connectionless Wi-Fi communication protocol developed by Espressif.



The result is a real-time, wireless, interactive LED "mirror" of the user's touches on the screen.



---



\## 🧩 Hardware Components



\- 2 × \*\*ESP32\*\* boards (e.g., ESP32 DevKit V1)

\- 1 × \*\*SPI TFT capacitive touchscreen\*\* (e.g., ILI9488 or XPT2046-based)

\- 1 × \*\*RGB LED strip\*\* (e.g., WS2812B), arranged in a rectangle to match screen layout

\- Power source (USB or battery)

\- Level shifters (if needed, depending on LED strip and touchscreen)



---



\## 📡 Communication



\### ESP-NOW



\- \*\*ESP-NOW\*\* is used for low-latency wireless communication between the two ESP32 devices.

\- Touch coordinates are sent from ESP32 #1 to ESP32 #2 in real-time.

\- No Wi-Fi router is required.



---



\## 🔌 Wiring Overview



\### ESP32 #1 (Touchscreen Controller)

\- Connects to TFT screen via SPI

\- Detects touch input

\- Sends data via ESP-NOW



\### ESP32 #2 (LED Controller)

\- Connects to RGB LED strip

\- Receives touch coordinates

\- Lights up the corresponding LED



---



\## 📷 Media



\### Demo Images



!\[Touchscreen Interface](images/1.jpg)



\### Video Demonstration



\[!\[Watch the demo](images/video\_thumb.jpg)](video/Main.mp4)

\*A video showing real-time interaction between screen and LED strip.\*



---



\## 🚀 How It Works



1\. \*\*User touches the screen\*\*.

2\. ESP32 #1 reads the touch coordinates (X, Y).

3\. Coordinates are \*\*mapped to the corresponding LED index\*\* based on screen-to-strip geometry.

4\. ESP32 #1 sends the LED index (and optionally color data) to ESP32 #2 using ESP-NOW.

5\. ESP32 #2 receives the command and lights up the specific LED.



---



\## 🧠 Customization Ideas



\- Multi-touch support (if the screen allows)

\- Dynamic color patterns based on swipe direction or pressure

\- Add animations or fade effects to LEDs

\- Two-way communication for feedback



---



\## ⚠️ Notes



\- Ensure both ESP32s are on the same Wi-Fi channel (even if not connected to a router).

\- ESP-NOW MAC address pairing is required.

\- Capacitive touchscreen libraries may require calibration.



---



\## 📁 File Structure





