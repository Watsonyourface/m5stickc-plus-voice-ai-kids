# M5StickC Plus Voice Assistant - v3.2

## What Is This?

A kid-friendly, screen-free AI voice assistant built for the M5StickC Plus that lets children ask questions and get answers through natural conversation - no tablets, no typing, just talking!

Perfect for ages 8-11 with built-in safety features and magical childhood preservation.

---

## Features

### Core Functionality
- 4-second voice recording - Just press and speak
- AI-powered answers - Uses OpenAI GPT-4o-mini for child-friendly responses
- Conversation memory - Remembers context for follow-up questions
- Word-by-word display - RSVP speed reading at 150 WPM (adjustable)
- Battery monitoring - Always shows charge level

### Physical Controls (The Cool Stuff!)
- **Tilt LEFT** - Slow down reading (100 WPM - cyan words)
- **Tilt RIGHT** - Speed up reading (250 WPM - yellow words)
- **SHAKE** - Replay the last answer anytime!
- **Button A** - Continue same topic (keeps conversation going)
- **Button B** - Start new topic (clears history)

### Safety & Magic
- **Content filtering** - Inappropriate topics redirect to trusted adults
- **Magic preservation** - Santa, Tooth Fairy, etc. stay magical!
- **Religious respect** - Neutral on all religions and cultures
- **Political neutrality** - No political opinions
- **English (UK) only** - No random language switches
- **Age-appropriate** - Simple, clear language for 8-11 year olds

---

## What You Need

### Hardware
- **M5StickC Plus** (ESP32-based mini IoT board)
- **USB-C cable** for programming and charging
- **2.4GHz WiFi network** (5GHz not supported)
- **Computer** (Mac/Windows/Linux) with Arduino IDE

### Software & Accounts
- **Arduino IDE** (1.8.x or 2.x)
- **M5Unified Library** (install via Arduino Library Manager)
- **OpenAI API Key** (get from platform.openai.com)
  - Free tier: ~3 questions/minute
  - Paid: Pay-as-you-go (~$0.01 per 10 questions)

---

## Quick Start Setup

### 1. Install Arduino IDE
Download from: https://www.arduino.cc/en/software

### 2. Install M5Unified Library
1. Open Arduino IDE
2. Go to **Tools → Manage Libraries**
3. Search for "M5Unified"
4. Click **Install**

### 3. Get OpenAI API Key
1. Go to https://platform.openai.com
2. Create account (if needed)
3. Go to **API Keys** section
4. Click **Create new secret key**
5. Copy the **entire key** (starts with `sk-proj-` or `sk-`)
   - WARNING: Copy the WHOLE thing including `sk-`
   - WARNING: Don't add spaces or line breaks

### 4. Configure the Code
Open `M5Voice_V3.ino` and find this section at the top:

```cpp
// ============================================================================
// CONFIGURATION - PASTE YOUR CREDENTIALS HERE
// ============================================================================

const char* OPENAI_API_KEY = "YOUR_API_KEY_HERE";  // <- Paste your FULL key here

// Optional: Pre-fill WiFi (or leave blank to use web setup)
const char* WIFI_SSID = "";      // <- Optional: Your WiFi name
const char* WIFI_PASSWORD = "";  // <- Optional: Your WiFi password

// ============================================================================
```

**Replace** `YOUR_API_KEY_HERE` with your actual OpenAI API key.

**Example:**
```cpp
const char* OPENAI_API_KEY = "sk-proj-abc123XYZ_your_actual_key_here";
```

**CRITICAL:** Make sure:
- You copied the **entire key** (usually 50+ characters)
- No extra spaces before or after the key
- Key is inside the quotes `""`
- Don't remove `sk-` at the start!

### 5. Upload to M5StickC Plus
1. Connect M5StickC Plus via USB-C
2. In Arduino IDE:
   - **Board**: Select "M5Stack-ATOMS3" or "ESP32 Dev Module"
   - **Port**: Select your device's port
3. Click **Upload** (arrow button)
4. Wait for "Upload Complete"

### 6. First Time WiFi Setup
If you didn't pre-fill WiFi credentials:

1. Device will show: **"WiFi Setup / Connect to: M5Voice-Setup"**
2. On phone/computer:
   - Connect to WiFi network: `M5Voice-Setup`
   - Open browser and go to: `192.168.4.1`
3. Enter your WiFi name and password
4. Device restarts and connects!

---

## How to Use

### Basic Operation

```
┌─────────────────────────────┐
│ Press A to ask!         78% │  <- Home screen shows battery
│                             │
│ Tips:                       │
│ Tilt <-- = slower           │
│ Tilt --> = faster           │
│ Shake = replay answer       │
└─────────────────────────────┘
```

1. **Press Button A** (bottom button)
2. **Wait for "Listening..."** screen
3. **Speak your question clearly** (2-4 seconds)
4. **Wait for "Thinking..."**
5. **Read the answer** word-by-word on screen
   - Tilt device left/right to adjust reading speed
   - Words change color based on speed (cyan=slow, yellow=fast)
6. **Shake to replay** if you missed it!
7. **Press A again** to ask follow-up question
8. **Press B** to start a new topic

### Button Controls

| Button | Action | What It Does |
|--------|--------|--------------|
| **A (short press)** | Ask/Continue | Record question (keeps conversation context) |
| **B (short press)** | New Topic | Record question (clears conversation history) |
| **B (hold 3 sec)** | Reset WiFi | Clears WiFi credentials and restarts |

### Physical Gestures

| Gesture | Effect | Visual Feedback |
|---------|--------|-----------------|
| **Tilt LEFT** | Slower reading (100 WPM) | Words turn **cyan** |
| **Flat** | Normal speed (150 WPM) | Words are **white** |
| **Tilt RIGHT** | Faster reading (250 WPM) | Words turn **yellow** |
| **Shake device** | Replay last answer | Shows "Replaying!" |

---

## Safety Features

### What Gets Blocked

The assistant **redirects to trusted adults** for:
- Sex, drugs, alcohol, violence
- Inappropriate content for children
- Religious judgment ("Is X religion bad?")
- Political opinions ("Is X politician good?")

**Example Response:**
> "Ask a parent or teacher about that."

### What Gets Preserved

The assistant **keeps the magic alive** for:
- Santa Claus
- Tooth Fairy
- Easter Bunny
- Other childhood magical figures

**Example Response:**
> "Many families have wonderful traditions about Santa! The magic of Christmas is special however you celebrate it."

### What Works Normally

The assistant **answers directly** for:
- Science questions ("Why is the sky blue?")
- History questions ("Who was the first person in space?")
- How things work ("How do airplanes fly?")
- Nature & animals ("Why do cats purr?")
- Facts about religions (without judgment)
- General knowledge appropriate for ages 8-11

---

## Battery Life

- **Active use** (asking questions): ~30-45 minutes
- **Idle** (screen on, WiFi connected): ~60-90 minutes
- **Recommendation**: Keep plugged in during extended use

**Battery Indicator:**
- **Green** (>50%) - Good to go
- **Yellow** (20-50%) - Will need charging soon
- **Red** (<20%) - Charge now!

Low battery warning appears automatically when below 15%.

---

## Troubleshooting

### "HTTP -1" Error (Can't Connect to OpenAI)

**Problem:** Device can't reach OpenAI API

**Solutions:**

1. **Check API Key** (Most Common!)
   - Make sure you pasted the **entire key** including `sk-`
   - Key should be 50+ characters long
   - No spaces before/after the key
   - Format: `const char* OPENAI_API_KEY = "sk-proj-abc123...";`

2. **Verify WiFi Connection**
   - Check serial monitor - does it say "Connected!" with an IP address?
   - Try resetting WiFi: Hold Button B for 3 seconds

3. **Check OpenAI Account**
   - Go to https://platform.openai.com
   - Make sure you have credit or free tier available
   - Verify API key is active (not expired/deleted)

4. **Test Connection**
   - Open serial monitor (Tools → Serial Monitor)
   - Set to 115200 baud
   - Look for "Connecting to OpenAI..."
   - Should see "HTTP response code: 200" (not -1)

### "Couldn't hear. Try again."
**Problem:** Microphone didn't pick up audio or transcription failed

**Solutions:**
- Speak louder and closer to device
- Reduce background noise
- Check WiFi connection is stable
- Speak clearly for 2-3 seconds (full 4 seconds not required)

### "WiFi lost! Reconnecting..."
**Problem:** Lost connection to WiFi

**Solutions:**
- Wait for automatic reconnection (10 seconds)
- Move closer to WiFi router
- Check router is working
- If fails, hold Button B for 3 seconds to reset WiFi

### Device won't connect to WiFi
**Problem:** Can't connect during initial setup

**Solutions:**
- Make sure you're using **2.4GHz WiFi** (not 5GHz)
- Check WiFi password is correct (case-sensitive)
- Try resetting: Hold Button B for 3 seconds
- Re-enter credentials at `192.168.4.1`

### "Memory low! Restarting..."
**Problem:** Device ran out of RAM

**Solutions:**
- This is normal after many questions
- Device automatically restarts
- Conversation history clears to free memory
- No data is lost permanently

### Shake detection too sensitive
**Problem:** Replays when you don't want it to

**Solutions:**
- In code, increase `SHAKE_THRESHOLD` from `2.0` to `2.5`
- Or increase `SHAKE_DEBOUNCE` from `1000` to `2000`

### Reading speed feels wrong
**Problem:** Too fast or too slow at default

**Solutions:**
- Adjust in real-time by tilting device!
- Or change defaults in code:
  ```cpp
  static const int DEFAULT_WPM = 150;  // Change this (100-250)
  static const int SLOW_WPM = 100;     // Change slow speed
  static const int FAST_WPM = 250;     // Change fast speed
  ```

### Getting Welsh/French responses
**Problem:** GPT responds in wrong language

**This should be fixed in v3.2!** System prompt forces English (UK).

If still happening:
- Make sure you uploaded v3.2 code
- Check your OpenAI account settings
- Report as bug

---

## Tips for Best Results

### For Kids
1. **Speak clearly** - Like talking to a friend
2. **Stay close** - About 15-30cm from device
3. **Ask complete questions** - "Why is the sky blue?" not just "Sky?"
4. **Use follow-ups** - Press A to continue the same topic
5. **Experiment with tilt** - Find your perfect reading speed!

### For Parents
1. **Start with easy questions** - Let kids get comfortable
2. **Demonstrate first** - Show them how it works
3. **Encourage curiosity** - There are no "bad" questions!
4. **Monitor battery** - Keep an eye on the percentage
5. **Teach follow-ups** - Help them learn to ask "Why?" and "How?"

### Question Examples to Get Started
- "Why do we have seasons?"
- "How do birds fly?"
- "What is gravity?"
- "Why do we dream?"
- "How does the internet work?"
- "What makes rainbows?"
- "Why do leaves change color?"

---

## Technical Specifications

| Spec | Details |
|------|---------|
| **Model** | M5StickC Plus (ESP32-PICO-V3-02) |
| **Screen** | 135x240 LCD (landscape orientation) |
| **Microphone** | I2S digital microphone (12kHz sample rate) |
| **Recording** | 4 seconds per question |
| **Battery** | 120mAh rechargeable Li-ion |
| **WiFi** | 2.4GHz only (802.11 b/g/n) |
| **API** | OpenAI Whisper (transcription) + GPT-4o-mini (responses) |
| **Memory** | Stores last 2 Q&A pairs (auto-clears when low) |
| **Response Limit** | 200 words maximum |
| **Reading Speed** | 100-250 WPM (default 150 WPM) |

---

## Privacy & Data

### What Gets Sent to OpenAI
- Voice recordings (transcribed, not stored by us)
- Questions and answers (for conversation context)
- Last 2 Q&A pairs for follow-up context

### What Stays on Device
- WiFi credentials (stored in encrypted flash)
- Current conversation (2 Q&A pairs max)
- Last response (for replay feature)

### What's NOT Stored
- Voice recordings (deleted after transcription)
- Long-term conversation history
- Personal information
- User profiles

**Note:** OpenAI may store API requests per their privacy policy. Check: https://openai.com/privacy

---

## Cost Estimate

### OpenAI API Costs (Pay-as-you-go)
- **Whisper (transcription)**: ~$0.006 per minute = ~$0.0004 per question
- **GPT-4o-mini (responses)**: ~$0.15 per 1M input tokens = ~$0.0003 per question
- **Total per question**: ~**$0.0007** (less than 0.1 pence!)

**Example usage:**
- 100 questions/day = **$0.07/day** = **£1.60/month**
- 50 questions/day = **$0.04/day** = **£0.80/month**

Very affordable for home use! Free tier may be sufficient for light usage.

---

## Advanced Configuration

### Changing Response Length
In `askGPT()` function, modify:
```cpp
String systemPrompt = "...Keep responses under 200 words...";  // Change limit
"\"max_tokens\":300"  // Adjust tokens accordingly
```

### Adjusting Tilt Sensitivity
In `getSpeedFromTilt()` function:
```cpp
if (accY < -0.3) {  // Change threshold (currently -0.3 to 0.3)
```

### Modifying Conversation Memory
At top of code:
```cpp
const int MAX_HISTORY = 2;  // Change to store more/fewer Q&A pairs
```
**Warning:** More history = more memory used = potential crashes!

### Custom Safety Responses
Edit the system prompt in `askGPT()` to customize how the assistant handles sensitive topics. Keep prompts short to avoid HTTP errors!

---

## Known Issues

1. **Battery life is short** - This is normal for ESP32 with WiFi. Keep plugged in for extended sessions.
2. **Occasional transcription errors** - Background noise affects accuracy. Speak clearly in quiet environment.
3. **Memory resets** - Device clears history when memory gets low. This is intentional to prevent crashes.
4. **2.4GHz WiFi only** - ESP32 limitation. 5GHz networks won't work.
5. **HTTP -1 errors** - Usually means API key is incorrect or not fully pasted. Copy the entire key including `sk-`

---

## Version History

### v3.2 (Current - January 2025)
- Added safety guardrails for inappropriate content
- Added magic preservation for childhood figures
- Added religious/cultural neutrality
- Added political neutrality
- Added battery monitoring and display
- Added WiFi reconnection handling
- Forced English (UK) responses only
- Improved home screen with battery indicator
- Optimized system prompt to prevent HTTP errors

### v3.1
- Added tilt controls for reading speed
- Added shake-to-replay feature
- Added helpful loading tips
- Added feature overview on home screen
- Set default speed to 150 WPM

### v3.0
- Added RSVP word-by-word display
- Added progress indicators
- Initial tilt/shake detection

### v2.7 (Previous stable)
- Conversation mode with memory
- Animated listening/thinking screens
- Persistent UI improvements

---

## Support & Community

### Getting Help
- **Issues with code?** Check Troubleshooting section above
- **HTTP -1 error?** Verify your API key is complete and correct
- **Hardware problems?** Check M5Stack documentation: https://docs.m5stack.com
- **OpenAI API issues?** Check status: https://status.openai.com

### Common Setup Mistakes
1. Not copying the full API key (missing characters)
2. Adding spaces around the API key
3. Leaving "YOUR_API_KEY_HERE" instead of replacing it
4. Using 5GHz WiFi instead of 2.4GHz
5. No credit on OpenAI account

### Feedback & Improvements
This is a learning project! Suggestions welcome for:
- Additional safety features
- New physical gestures
- Better UX for kids
- Battery optimization
- Other cool features!

---

## License & Credits

### Credits
- **Hardware**: M5Stack M5StickC Plus
- **AI**: OpenAI Whisper + GPT-4o-mini
- **Development**: Built with Arduino + M5Unified library
- **Concept**: Screen-free learning for curious kids!

### License
This project is open-source for educational purposes. 

**Important:** You need your own OpenAI API key (not included). API usage subject to OpenAI's terms and pricing.

---

## Learning Opportunities

This project is great for teaching:
- **Kids**: How AI works, asking good questions, speed reading
- **Families**: STEM concepts, programming basics, IoT devices
- **Makers**: Arduino, ESP32, API integration, voice interfaces

---

## Important Notes

1. **Adult supervision recommended** for initial setup
2. **Not a replacement for parental guidance** - Redirects sensitive topics to adults
3. **Internet required** - Doesn't work offline
4. **Costs apply** - OpenAI API is pay-as-you-go (very cheap but not free)
5. **Privacy** - Voice recordings sent to OpenAI for processing
6. **Age appropriate** - Designed for ages 8-11
7. **API Key Security** - Make sure you paste the complete key including `sk-`

---

## Ready to Go!

Your M5StickC Plus Voice Assistant is now ready to inspire curiosity, answer questions, and make learning fun!

**First question to try:** "How does a rainbow work?"

**If you get HTTP -1 errors:** Double-check your API key is complete!

Happy learning!

---

**Version 3.2 | January 2025**
