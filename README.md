# M5StickC Plus Voice AI Assistant for Kids

A screen-free voice AI learning companion that lets kids explore their curiosity through natural conversations. Built for the **M5StickC Plus**, this pocket-sized device encourages learning without the distraction of tablets, phones, or keyboards.

Designed for real children, real questions, and long-running stability on ESP32 hardware.

---

## Key Features

- **4-Second Voice Recording**  
  Optimised 12kHz audio capture gives kids enough time to form complete questions without rushing.

- **Conversation Mode (Memory-Safe)**  
  Remembers the *last question only*, allowing natural follow-ups like “tell me more” or “give an example”, without crashing the device.

- **Kid-Friendly AI Responses**  
  Tuned for curious 8–11 year olds. Clear explanations, relatable examples, and a strict **30-word maximum**.

- **Friendly Face Animation**  
  Simple visual feedback shows when the device is listening or thinking using a calm, inclusive character.

- **Two-Button Interface**  
  - Button A: Ask or continue the current topic  
  - Button B: Start a new topic or reset WiFi

- **Persistent Display**  
  Answers stay on screen until the next action. No missed responses.

- **Memory-Optimised for ESP32**  
  Automatic memory management prevents crashes and allows indefinite use.

- **WiFi Setup Portal**  
  Easy configuration via a local web interface at `192.168.4.1`.

- **Secure Configuration Section**  
  All credentials live in one clearly marked block for easy copy and paste.

---

## Why This Exists

This project was created to give my own kids (ages 8, 9, and 11) a way to use AI for learning **without**:

- Screen addiction from tablets
- Typing barriers for younger learners
- Distracting user interfaces or notifications
- Needing constant adult supervision

Kids can ask:

> “What is gravity?”  
> “Tell me more.”  
> “Can you give me an example?”

Just like talking to a patient tutor.

---

## Inspiration and Credits

This project was inspired by **@organised’s M5Stick OpenAI voice assistant**.

I rebuilt it from scratch with a focus on education, stability, and child-friendly interaction.

### What I Added

- Conversation context with automatic memory management
- Extended recording time (4 seconds at 12kHz)
- Kid-optimised AI responses (simple language, 30-word limit)
- Friendly listening / thinking animations
- Persistent answer display with button prompts
- Memory leak prevention and auto-recovery
- Clean, clearly marked configuration section

### What Stayed from the Original

- WiFi access point setup concept
- Core M5StickC + OpenAI integration approach
- Recording and transcription workflow

Full credit to **@organised** for the foundation.

---

## Technical Specifications

- **Hardware:** M5StickC Plus (ESP32-PICO-V3-02)
- **Audio:** 4-second recording at 12kHz via built-in I2S microphone
- **AI Services:**
  - OpenAI Whisper (speech-to-text)
  - GPT-4o-mini (responses)
- **Memory:**
  - 96KB audio buffer
  - Automatic heap monitoring and cleanup
- **Display:** 240 × 135 LCD (landscape)
- **Conversation History:** Last 1 Q&A pair only
- **Stability:** Runs indefinitely without manual restarts

---

## What You’ll Need

- M5StickC Plus device
- OpenAI API key (pay-as-you-go)
- 2.4GHz WiFi network (5GHz not supported)
- USB-C data cable
- Arduino IDE with ESP32 support

---

## Quick Start

### 1. Install Dependencies

#### Arduino IDE
1. Install Arduino IDE
2. Add ESP32 board support:
   - File → Preferences
   - Additional Board Manager URLs:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Tools → Board → Boards Manager → Install **esp32**

#### Library
- Install **M5Unified** via Library Manager

---

### 2. Configure Credentials

Open `M5Voice_v2.7.ino` and edit the configuration section:

```cpp
// ============================================================================
// CONFIGURATION - PASTE YOUR CREDENTIALS HERE
// ============================================================================

const char* OPENAI_API_KEY = "sk-proj-your-key-here";

// Optional: leave blank to use WiFi setup portal
const char* WIFI_SSID = "YourWiFiName";
const char* WIFI_PASSWORD = "YourWiFiPassword";
