# Kiddo - Interactive Story Device

A fun, interactive storytelling device built for kids using an ESP32 with a touchscreen display. Create and explore stories with different paths and choices, all designed to spark imagination and creativity.

## Hardware Requirements

This project uses affordable, easily accessible hardware:

### Core Components
- **ESP32-2432S028R** - ILI9341 device with 2.8" touchscreen (under $20)
- **Mini Speaker** - 8ohm 1W (optional, for audio feedback)

### 3D Printed Case
- **Case Design**: [CYD Case on MakerWorld](https://makerworld.com/en/models/772736-cyd-case?from=search#profileId-708840)
- Perfectly fits the ESP32-2432S028R board and provides a kid-friendly enclosure

## Features

- **Interactive Stories**: Choose-your-own-adventure style stories with branching paths
- **Multilingual Support**: Stories available in multiple languages (English, Portuguese)
- **Touch Interface**: Intuitive touch controls perfect for kids
- **Story Management**: Download and sync stories from remote catalogs
- **Audio Support**: Optional speaker for sound effects and feedback
- **WiFi Connectivity**: Download new stories and sync content

## Inspiration

This project was inspired by [Aura](https://github.com/Surrey-Homeware/Aura) - a similar concept that sparked the idea for creating a personalized storytelling device.

## Story Creation

Stories are created using AI tools to generate engaging, age-appropriate content with multiple branching paths. Each story is structured as JSON files with nodes and choices, making it easy to create complex narrative experiences.

## Getting Started

### Prerequisites
- PlatformIO IDE or PlatformIO Core
- ESP32-2432S028R board
- USB cable for programming

### Installation
1. Clone this repository
2. Open the project in PlatformIO
3. Build and upload to your ESP32 device
4. Configure WiFi settings through the device interface

### Configuration
- Set up WiFi credentials on first boot
- Choose your preferred language
- Download initial story content

## Technical Stack

### Libraries Used
- **[LVGL](https://lvgl.io/)** - Modern UI library for embedded systems
- **[ArduinoJson](https://arduinojson.org/)** - JSON parsing and serialization
- **[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)** - Display driver for ESP32
- **[XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)** - Touch input handling
- **HttpClient** - Network communication for story downloads

### Architecture
- **Modular Design**: Separate modules for UI, story engine, network, and storage
- **Story Engine**: JSON-based story format with node and choice structure
- **Remote Catalog**: Download and sync stories from online sources
- **Local Storage**: SPIFFS-based file system for story caching
- **Multilingual**: Centralized language filtering and management


## Thanks & Credits

### Inspiration
- **[Aura Project](https://github.com/Surrey-Homeware/Aura)** - Original inspiration for using the Cheap Yellow Board

### Technical Resources
- **[LVGL](https://lvgl.io/)** - Amazing UI library that made ESP32 interface development much easier
- **[witnessmenow's CYD Github](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)** - Essential reference for ESP32-2432S028R development
- **[witnessmenow's ESP32 Web Flashing Tutorial](https://www.youtube.com/c/witnessmenow)** - Helpful deployment guidance
- **[Random Nerd Tutorials](https://randomnerdtutorials.com/)** - Excellent ESP32 and LVGL setup tutorials

### Hardware Design
- **[@hedge on MakerWorld](https://makerworld.com/en/@hedge)** - Excellent 3D printed case design that perfectly fits the ESP32-2432S028R

### Libraries
- **[LVGL](https://lvgl.io/)** - Graphics and UI framework
- **[ArduinoJson](https://arduinojson.org/)** - JSON handling
- **[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)** - Display driver
- **[XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)** - Touch input

## Personal Note

This project was created as a labor of love for my daughter Zoe, with the goal of building an engaging, educational device that encourages reading and imagination. The use of AI tools in development allowed a non-C++ developer to create something meaningful and functional.

## License

This project is open source. Check the LICENSE file for details.

## Contributing

Contributions are welcome! Whether it's bug fixes, new features, or additional stories, feel free to open issues or submit pull requests.

---

*Built with ❤️ for Zoe and all the young storytellers out there.*
