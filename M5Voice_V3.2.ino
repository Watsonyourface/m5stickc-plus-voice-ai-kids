/*
 * M5StickC Plus Voice Assistant - v3.2
 * 
 * Hardware: M5StickC Plus (ESP32-PICO-V3-02)
 * 
 * NEW in v3.2:
 * - Safety guardrails for inappropriate questions
 * - Magic preservation (Santa, Tooth Fairy, etc.)
 * - Religious and cultural respect
 * - Political neutrality
 * - Battery level display on home screen
 * - WiFi reconnection handling
 * - Guaranteed English (UK) responses only
 * - Optimized system prompt (prevents HTTP errors)
 * 
 * Features:
 * - 4-second recording (12kHz)
 * - Conversation mode - Button A continues topic!
 * - Tilt left/right to control reading speed
 * - Shake to replay last answer
 * - Child-safe content filtering
 * - Battery monitoring
 * 
 * Button Controls:
 * - Button A (click): CONTINUE same topic (remembers context)
 * - Button B (click): Ask a NEW question (starts fresh)
 * - Button B (hold 3 sec): Reset WiFi credentials
 * 
 * Physical Controls:
 * - Tilt LEFT: Slower reading (100 WPM)
 * - Flat: Normal speed (150 WPM)
 * - Tilt RIGHT: Faster reading (250 WPM)
 * - SHAKE: Replay last answer
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>

// ============================================================================
// CONFIGURATION - PASTE YOUR CREDENTIALS HERE
// ============================================================================

const char* OPENAI_API_KEY = "YOUR_API_KEY_HERE";

// Optional: Pre-fill WiFi (or leave blank to use web setup at 192.168.4.1)
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

// ============================================================================
// END CONFIGURATION - Don't change anything below this line!
// ============================================================================

// ============================================================================
// DISPLAY SETTINGS
// ============================================================================

static const int WIDTH  = 240;
static const int HEIGHT = 135;

// RSVP Display Settings with Tilt Control
static const int DEFAULT_WPM = 150;      // Default speed (slower for kids)
static const int SLOW_WPM = 100;         // Tilted left
static const int FAST_WPM = 250;         // Tilted right
static int currentWPM = DEFAULT_WPM;     // Current reading speed

// Shake Detection Settings
static const float SHAKE_THRESHOLD = 2.0;  // Acceleration threshold for shake
static unsigned long lastShakeTime = 0;    // Debounce shakes
static const int SHAKE_DEBOUNCE = 1000;    // 1 second between shakes

// ============================================================================
// AUDIO RECORDING SETTINGS
// ============================================================================

static const int SAMPLE_RATE = 12000;
static const int RECORD_SECONDS = 4;
static const int RECORD_SAMPLES = SAMPLE_RATE * RECORD_SECONDS;
static int16_t* audioBuffer = nullptr;

// ============================================================================
// CONVERSATION HISTORY & REPLAY
// ============================================================================

const int MAX_HISTORY = 2;
String conversationHistory[MAX_HISTORY];
int historyCount = 0;
bool conversationMode = false;

// Store last response for replay
String lastResponse = "";

// ============================================================================
// WIFI & WEB SERVER
// ============================================================================

Preferences prefs;
WebServer server(80);
String savedSSID = "";
String savedPass = "";
bool apMode = false;

// ============================================================================
// HTML PAGES FOR WIFI SETUP
// ============================================================================

const char* setupPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; background: #1a1a2e; color: #fff; padding: 20px; }
    .container { max-width: 300px; margin: 0 auto; }
    h1 { color: #0f0; font-size: 24px; }
    input { width: 100%; padding: 12px; margin: 8px 0; box-sizing: border-box; 
            border-radius: 8px; border: none; font-size: 16px; }
    input[type=submit] { background: #0f0; color: #000; cursor: pointer; 
                         font-weight: bold; }
    input[type=submit]:hover { background: #0c0; }
  </style>
</head>
<body>
  <div class="container">
    <h1>M5 Voice Setup</h1>
    <form action="/save" method="POST">
      <input type="text" name="ssid" placeholder="WiFi Name" required>
      <input type="password" name="pass" placeholder="WiFi Password" required>
      <input type="submit" value="Connect">
    </form>
  </div>
</body>
</html>
)rawliteral";

const char* successPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; background: #1a1a2e; color: #fff; padding: 20px; 
           text-align: center; }
    h1 { color: #0f0; }
  </style>
</head>
<body>
  <h1>Saved!</h1>
  <p>Device is restarting...</p>
</body>
</html>
)rawliteral";

// ============================================================================
// IMU FUNCTIONS (Tilt & Shake Detection)
// ============================================================================

// Map tilt to reading speed (LEFT/RIGHT based on Y-axis)
int getSpeedFromTilt() {
  M5.Imu.update();
  
  // Get acceleration data using pointers
  float accX, accY, accZ;
  M5.Imu.getAccel(&accX, &accY, &accZ);
  
  // When buttons are top/bottom (landscape with B on top, A on bottom):
  // Tilt LEFT (left side down) = accY is negative = SLOWER
  // Tilt RIGHT (right side down) = accY is positive = FASTER
  
  if (accY < -0.3) {
    // Tilted LEFT (left side down) = slower
    return SLOW_WPM;
  } else if (accY > 0.3) {
    // Tilted RIGHT (right side down) = faster
    return FAST_WPM;
  } else {
    // Flat = normal speed
    return DEFAULT_WPM;
  }
}

// Detect shake
bool detectShake() {
  M5.Imu.update();
  
  // Get acceleration data using pointers
  float accX, accY, accZ;
  M5.Imu.getAccel(&accX, &accY, &accZ);
  
  // Calculate total acceleration magnitude
  float totalAccel = sqrt(accX*accX + accY*accY + accZ*accZ);
  
  // Check if shaken (and not too soon after last shake)
  if (totalAccel > SHAKE_THRESHOLD && 
      (millis() - lastShakeTime) > SHAKE_DEBOUNCE) {
    lastShakeTime = millis();
    return true;
  }
  
  return false;
}

// ============================================================================
// WIFI HELPER FUNCTION
// ============================================================================

bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return true;
  
  Serial.println("WiFi disconnected! Attempting reconnection...");
  drawScreen("WiFi lost!\n\nReconnecting...");
  
  WiFi.begin(savedSSID.c_str(), savedPass.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nReconnected!");
    return true;
  }
  
  Serial.println("\nReconnection failed!");
  drawScreen("WiFi reconnection\nfailed!\n\nPlease restart");
  delay(3000);
  return false;
}

// ============================================================================
// WORD SPLITTING FUNCTION
// ============================================================================

int splitIntoWords(const String &text, String* words, int maxWords) {
  int wordCount = 0;
  String currentWord = "";
  
  for (unsigned int i = 0; i < text.length(); i++) {
    char c = text[i];
    
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
      if (currentWord.length() > 0 && wordCount < maxWords) {
        words[wordCount++] = currentWord;
        currentWord = "";
      }
    } else {
      currentWord += c;
    }
  }
  
  if (currentWord.length() > 0 && wordCount < maxWords) {
    words[wordCount++] = currentWord;
  }
  
  return wordCount;
}

// ============================================================================
// RSVP DISPLAY WITH TILT CONTROL & SHAKE TO REPLAY
// ============================================================================

void displayRSVP(const String &text) {
  const int MAX_WORDS = 250;
  String* words = new String[MAX_WORDS];
  int wordCount = splitIntoWords(text, words, MAX_WORDS);
  
  Serial.printf("Displaying %d words in RSVP mode\n", wordCount);
  
  // Display each word
  for (int i = 0; i < wordCount; i++) {
    M5.update();
    
    // Check for shake during playback (skip to end)
    if (detectShake()) {
      Serial.println("Shake detected during playback - skipping to end");
      break;
    }
    
    // Get current speed from tilt
    currentWPM = getSpeedFromTilt();
    int delayMs = 60000 / currentWPM;
    
    // Clear screen
    M5.Display.fillScreen(TFT_BLACK);
    
    // Display word (large, centered)
    M5.Display.setTextDatum(CC_DATUM);
    M5.Display.setTextSize(3);
    
    // Color based on speed
    uint16_t wordColor = TFT_WHITE;
    if (currentWPM <= SLOW_WPM) wordColor = TFT_CYAN;        // Slow = cyan
    else if (currentWPM >= FAST_WPM) wordColor = TFT_YELLOW; // Fast = yellow
    
    M5.Display.setTextColor(wordColor, TFT_BLACK);
    M5.Display.drawString(words[i], WIDTH/2, HEIGHT/2 - 15);
    
    // Progress indicator
    M5.Display.setTextSize(1);
    M5.Display.setTextDatum(BC_DATUM);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    String progress = String(i + 1) + " of " + String(wordCount);
    M5.Display.drawString(progress, WIDTH/2, HEIGHT - 5);
    
    // Wait based on current speed
    delay(delayMs);
  }
  
  // Show completion screen
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(CC_DATUM);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Display.drawString("Done!", WIDTH/2, HEIGHT/2 - 30);
  
  // Button hints
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawString("A=More  B=New", WIDTH/2, HEIGHT/2);
  
  // Shake hint
  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Display.drawString("Shake=Replay", WIDTH/2, HEIGHT/2 + 15);
  
  delete[] words;
  
  Serial.println("RSVP display complete");
}

// ============================================================================
// CONVERSATION MANAGEMENT FUNCTIONS
// ============================================================================

void addToHistory(const String &role, const String &content) {
  if (ESP.getFreeHeap() < 50000) {
    Serial.println("Low memory detected! Clearing history to prevent crash.");
    clearHistory();
    return;
  }
  
  if (historyCount < MAX_HISTORY) {
    conversationHistory[historyCount] = "\"role\":\"" + role + "\",\"content\":\"" + content + "\"";
    historyCount++;
  } else {
    for (int i = 0; i < MAX_HISTORY - 1; i++) {
      conversationHistory[i] = conversationHistory[i + 1];
    }
    conversationHistory[MAX_HISTORY - 1] = "\"role\":\"" + role + "\",\"content\":\"" + content + "\"";
  }
  
  Serial.println("\n=== Conversation History ===");
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  for (int i = 0; i < historyCount; i++) {
    Serial.println(conversationHistory[i]);
  }
  Serial.println("============================\n");
}

void clearHistory() {
  historyCount = 0;
  conversationMode = false;
  Serial.println("Conversation history cleared - starting fresh topic!");
}

String getHistoryJSON() {
  String history = "";
  for (int i = 0; i < historyCount; i++) {
    if (i > 0) history += ",";
    history += "{" + conversationHistory[i] + "}";
  }
  return history;
}

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================

void drawScreen(const String &text) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setTextSize(1);
  
  int y = 10;
  int startPos = 0;
  
  for (unsigned int i = 0; i <= text.length(); i++) {
    if (i == text.length() || text[i] == '\n') {
      String line = text.substring(startPos, i);
      M5.Display.drawString(line, 10, y);
      y += 18;
      startPos = i + 1;
    }
  }
}

void drawListening(int seconds) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  
  M5.Display.drawString("Listening...", 60, 10);
  
  M5.Display.setTextSize(1);
  M5.Display.drawString(String(seconds) + " sec", 100, 35);
  
  int earSize = 3 + (seconds % 2) * 2;
  
  M5.Display.fillCircle(90, 70, 4, TFT_WHITE);
  M5.Display.fillCircle(130, 70, 4, TFT_WHITE);
  
  M5.Display.drawCircle(110, 80, 15, TFT_WHITE);
  M5.Display.fillRect(95, 65, 30, 15, TFT_BLACK);
  
  M5.Display.fillCircle(70, 70, earSize, TFT_WHITE);
  M5.Display.fillCircle(150, 70, earSize, TFT_WHITE);
}

// Thinking screen with helpful tip
void drawThinking() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setTextDatum(TL_DATUM);
  
  M5.Display.drawString("Thinking...", 60, 10);
  
  // Thinking face
  M5.Display.drawLine(85, 70, 95, 70, TFT_WHITE);
  M5.Display.drawLine(125, 70, 135, 70, TFT_WHITE);
  
  M5.Display.drawCircle(110, 85, 12, TFT_WHITE);
  M5.Display.fillRect(98, 70, 24, 12, TFT_BLACK);
  
  int offset = (millis() / 300) % 3;
  if (offset >= 0) M5.Display.fillCircle(150, 50, 2, TFT_WHITE);
  if (offset >= 1) M5.Display.fillCircle(160, 45, 3, TFT_WHITE);
  if (offset >= 2) M5.Display.fillCircle(172, 42, 4, TFT_WHITE);
  
  // Helpful tip
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
  M5.Display.drawString("Tip: Tilt <-- slow", 5, 105);
  M5.Display.drawString("     Tilt --> fast", 5, 118);
}

// Transcribing screen with helpful tip
void drawTranscribing() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setTextDatum(TL_DATUM);
  
  M5.Display.drawString("Transcribing", 55, 40);
  
  // Animated dots
  int dots = (millis() / 500) % 4;
  String dotStr = "";
  for (int i = 0; i < dots; i++) dotStr += ".";
  M5.Display.drawString(dotStr + "   ", 55, 65);
  
  // Helpful tip
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Display.drawString("Tip: Shake to replay", 5, 105);
  M5.Display.drawString("answers anytime!", 5, 118);
}

// Home screen with battery display and feature overview
void showReadyScreen() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setTextSize(1);
  
  // Show battery level in top right
  int batteryLevel = M5.Power.getBatteryLevel();
  M5.Display.setTextDatum(TR_DATUM); // Top right alignment
  
  // Color based on level
  if (batteryLevel > 50) M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  else if (batteryLevel > 20) M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
  else M5.Display.setTextColor(TFT_RED, TFT_BLACK);
  
  M5.Display.drawString(String(batteryLevel) + "%", WIDTH - 5, 5);
  
  // Reset to normal alignment
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  
  if (conversationMode) {
    M5.Display.drawString("Same Topic Mode", 5, 5);
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Display.drawString("A = Keep going", 5, 25);
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.drawString("B = New topic", 5, 40);
  } else {
    M5.Display.setTextSize(2);
    M5.Display.drawString("Press A", 70, 5);
    M5.Display.drawString("to ask!", 75, 25);
    M5.Display.setTextSize(1);
  }
  
  // Feature overview
  M5.Display.drawLine(0, 60, WIDTH, 60, TFT_DARKGREY);
  M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
  M5.Display.drawString("Tips:", 5, 65);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawString("Tilt <-- = slower", 5, 80);
  M5.Display.drawString("Tilt --> = faster", 5, 93);
  M5.Display.drawString("Shake = replay answer", 5, 106);
  M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
  M5.Display.drawString("Hold B 3s = reset WiFi", 5, 119);
}

// ============================================================================
// WIFI SETUP FUNCTIONS
// ============================================================================

void handleRoot() {
  Serial.println("Serving setup page");
  server.send(200, "text/html", setupPage);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  
  Serial.println("Received credentials:");
  Serial.println("  SSID: " + ssid);
  Serial.println("  Pass: " + String(pass.length()) + " chars");
  
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  
  Serial.println("Credentials saved to Preferences");
  
  server.send(200, "text/html", successPage);
  
  delay(2000);
  ESP.restart();
}

void startAPMode() {
  Serial.println("\n========== STARTING AP MODE ==========");
  
  apMode = true;
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP("M5Voice-Setup");
  
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(ip);
  
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  
  Serial.println("Web server started");
  Serial.println("=======================================\n");
  
  drawScreen("WiFi Setup\n\nConnect to:\nM5Voice-Setup\n\nThen go to:\n192.168.4.1");
}

bool connectToWiFi() {
  prefs.begin("wifi", true);
  savedSSID = prefs.getString("ssid", WIFI_SSID);
  savedPass = prefs.getString("pass", WIFI_PASSWORD);
  prefs.end();
  
  Serial.println("\n========== WIFI CONNECTION ==========");
  Serial.println("Stored SSID: " + savedSSID);
  Serial.println("Stored pass: " + String(savedPass.length()) + " chars");
  
  if (savedSSID.length() == 0) {
    Serial.println("No credentials stored");
    Serial.println("======================================\n");
    return false;
  }
  
  drawScreen("Connecting to\n" + savedSSID + "...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPass.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("======================================\n");
    return true;
  }
  
  Serial.println("\nFailed to connect");
  Serial.println("======================================\n");
  return false;
}

void resetCredentials() {
  Serial.println("\n========== RESETTING CREDENTIALS ==========");
  
  drawScreen("Resetting...");
  
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  
  Serial.println("Credentials cleared, restarting...");
  Serial.println("============================================\n");
  
  delay(1000);
  ESP.restart();
}

// ============================================================================
// AUDIO RECORDING FUNCTIONS
// ============================================================================

bool recordAudio() {
  Serial.println("\n========== RECORDING ==========");
  
  if (!audioBuffer) {
    Serial.println("ERROR: Audio buffer not allocated!");
    return false;
  }

  Serial.println("Starting recording...");
  
  int samplesPerSecond = SAMPLE_RATE;
  
  for (int sec = 0; sec < RECORD_SECONDS; sec++) {
    Serial.printf("Recording second %d/%d...\n", sec + 1, RECORD_SECONDS);
    
    M5.Mic.record(&audioBuffer[sec * samplesPerSecond], samplesPerSecond, SAMPLE_RATE);
    
    while (M5.Mic.isRecording()) {
      drawListening(RECORD_SECONDS - sec);
      delay(50);
    }
  }
  
  Serial.println("Recording complete");

  int16_t minVal = 32767, maxVal = -32768;
  int64_t sum = 0;
  for (int i = 0; i < RECORD_SAMPLES; i++) {
    if (audioBuffer[i] < minVal) minVal = audioBuffer[i];
    if (audioBuffer[i] > maxVal) maxVal = audioBuffer[i];
    sum += abs(audioBuffer[i]);
  }
  Serial.printf("Audio stats: min=%d, max=%d, avg=%lld\n", 
                minVal, maxVal, sum / RECORD_SAMPLES);
  Serial.println("================================\n");
  
  return true;
}

void createWavHeader(uint8_t* header, int dataSize) {
  int fileSize = dataSize + 36;
  int byteRate = SAMPLE_RATE * 1 * 16 / 8;
  int blockAlign = 1 * 16 / 8;

  memcpy(header, "RIFF", 4);
  header[4] = fileSize & 0xFF;
  header[5] = (fileSize >> 8) & 0xFF;
  header[6] = (fileSize >> 16) & 0xFF;
  header[7] = (fileSize >> 24) & 0xFF;
  memcpy(header + 8, "WAVE", 4);
  memcpy(header + 12, "fmt ", 4);
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
  header[20] = 1; header[21] = 0;
  header[22] = 1; header[23] = 0;
  header[24] = SAMPLE_RATE & 0xFF;
  header[25] = (SAMPLE_RATE >> 8) & 0xFF;
  header[26] = (SAMPLE_RATE >> 16) & 0xFF;
  header[27] = (SAMPLE_RATE >> 24) & 0xFF;
  header[28] = byteRate & 0xFF;
  header[29] = (byteRate >> 8) & 0xFF;
  header[30] = (byteRate >> 16) & 0xFF;
  header[31] = (byteRate >> 24) & 0xFF;
  header[32] = blockAlign; header[33] = 0;
  header[34] = 16; header[35] = 0;
  memcpy(header + 36, "data", 4);
  header[40] = dataSize & 0xFF;
  header[41] = (dataSize >> 8) & 0xFF;
  header[42] = (dataSize >> 16) & 0xFF;
  header[43] = (dataSize >> 24) & 0xFF;
}

// ============================================================================
// OPENAI API FUNCTIONS
// ============================================================================

String transcribeAudio() {
  Serial.println("\n========== TRANSCRIBING ==========");
  
  // Check WiFi connection
  if (!ensureWiFiConnected()) {
    return "WiFi error";
  }
  
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(60);

  Serial.println("Connecting to api.openai.com...");
  if (!client.connect("api.openai.com", 443)) {
    Serial.println("ERROR: Connection failed!");
    return "Connection failed";
  }
  Serial.println("Connected");

  int audioDataSize = RECORD_SAMPLES * sizeof(int16_t);
  uint8_t wavHeader[44];
  createWavHeader(wavHeader, audioDataSize);

  String boundary = "----ESP32Boundary";
  
  String bodyStart = "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n";
  bodyStart += "Content-Type: audio/wav\r\n\r\n";
  
  String bodyEnd = "\r\n--" + boundary + "\r\n";
  bodyEnd += "Content-Disposition: form-data; name=\"model\"\r\n\r\n";
  bodyEnd += "whisper-1\r\n";
  bodyEnd += "--" + boundary + "--\r\n";

  int contentLength = bodyStart.length() + 44 + audioDataSize + bodyEnd.length();
  Serial.printf("Content length: %d bytes (audio: %d bytes)\n", contentLength, audioDataSize);

  Serial.println("Sending request headers...");
  client.print("POST /v1/audio/transcriptions HTTP/1.1\r\n");
  client.print("Host: api.openai.com\r\n");
  client.print("Authorization: Bearer " + String(OPENAI_API_KEY) + "\r\n");
  client.print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
  client.print("Content-Length: " + String(contentLength) + "\r\n");
  client.print("Connection: close\r\n\r\n");

  Serial.println("Sending audio data...");
  client.print(bodyStart);
  client.write(wavHeader, 44);
  
  int chunkSize = 4096;
  uint8_t* ptr = (uint8_t*)audioBuffer;
  int remaining = audioDataSize;
  int sent = 0;
  
  unsigned long lastUpdate = millis();
  
  while (remaining > 0) {
    int toSend = min(chunkSize, remaining);
    client.write(ptr, toSend);
    ptr += toSend;
    remaining -= toSend;
    sent += toSend;
    
    if (millis() - lastUpdate > 200) {
      drawTranscribing();
      lastUpdate = millis();
    }
    
    delay(1);
  }
  
  client.print(bodyEnd);
  Serial.println("Request sent, waiting for response...");

  unsigned long timeout = millis();
  while (!client.available()) {
    if (millis() - timeout > 60000) {
      Serial.println("ERROR: Timeout waiting for response!");
      client.stop();
      return "Timeout";
    }
    if (millis() - lastUpdate > 200) {
      drawTranscribing();
      lastUpdate = millis();
    }
    delay(100);
  }

  Serial.println("Response received, reading headers...");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.println("  " + line);
    if (line == "\r") break;
  }

  String response = client.readString();
  client.stop();

  Serial.println("Whisper response body:");
  Serial.println(response);

  int textIdx = response.indexOf("\"text\"");
  if (textIdx < 0) {
    Serial.println("ERROR: No 'text' field in response!");
    return "No transcription";
  }
  
  int start = response.indexOf('"', textIdx + 6);
  if (start < 0) {
    Serial.println("ERROR: Parse error!");
    return "Parse error";
  }
  start++;
  
  String result;
  bool escaped = false;
  for (unsigned int i = start; i < response.length(); i++) {
    char c = response[i];
    if (escaped) {
      result += c;
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == '"') {
      break;
    } else {
      result += c;
    }
  }
  
  Serial.println("Transcription: " + result);
  Serial.println("==================================\n");
  
  return result;
}

String askGPT(const String &question) {
  Serial.println("\n========== ASKING GPT ==========");
  Serial.println("Question: " + question);
  Serial.println("Conversation mode: " + String(conversationMode ? "YES" : "NO"));
  
  // Check WiFi connection
  if (!ensureWiFiConnected()) {
    return "WiFi error";
  }
  
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();

  Serial.println("Connecting to OpenAI...");
  http.begin(client, "https://api.openai.com/v1/chat/completions");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + OPENAI_API_KEY);
  http.setTimeout(90000);

  String escaped = question;
  escaped.replace("\\", "\\\\");
  escaped.replace("\"", "\\\"");
  escaped.replace("\n", " ");

  String messages;
  
  if (conversationMode && historyCount > 0) {
    messages = getHistoryJSON() + ",";
    messages += "{\"role\":\"user\",\"content\":\"" + escaped + "\"}";
  } else {
    messages = "{\"role\":\"user\",\"content\":\"" + escaped + "\"}";
  }

  // OPTIMIZED SYSTEM PROMPT - Shorter to prevent HTTP errors
  String systemPrompt = "You are a helpful assistant for children aged 8-11. "
                        "Respond ONLY in English (UK). "
                        "Keep responses under 200 words. "
                        "Use simple language. "
                        "For inappropriate topics (sex, drugs, violence), say: 'Ask a parent or teacher about that.' "
                        "For Santa/Tooth Fairy questions, preserve the magic - never say they're not real. "
                        "For religious questions asking if beliefs are 'bad' or 'wrong', say: 'All beliefs are important to those who follow them. Ask your family!' "
                        "Stay neutral on politics.";

  String body =
  "{"
    "\"model\":\"gpt-4o-mini\","
    "\"messages\":["
      "{\"role\":\"system\",\"content\":\"" + systemPrompt + "\"},"
      + messages +
    "],"
    "\"max_tokens\":300"
  "}";

  Serial.println("Sending request...");
  
  int httpCode = http.POST(body);
  Serial.printf("HTTP response code: %d\n", httpCode);

  if (httpCode != 200) {
    String error = http.getString();
    Serial.println("Error response: " + error);
    http.end();
    return "HTTP " + String(httpCode);
  }

  String resp = http.getString();
  http.end();

  Serial.println("GPT response:");
  Serial.println(resp);

  int contentIdx = resp.indexOf("\"content\"");
  if (contentIdx < 0) {
    Serial.println("ERROR: No 'content' in response!");
    return "No content";
  }

  int start = resp.indexOf(":", contentIdx);
  if (start < 0) {
    Serial.println("ERROR: Parse error!");
    return "Parse error";
  }
  start = resp.indexOf("\"", start) + 1;

  String result;
  bool esc = false;
  for (unsigned int i = start; i < resp.length(); i++) {
    char c = resp[i];
    if (esc) {
      if (c == 'n') result += ' ';
      else if (c == 'u') { i += 4; result += '-'; }
      else result += c;
      esc = false;
    } else if (c == '\\') {
      esc = true;
    } else if (c == '"') {
      break;
    } else {
      result += c;
    }
  }

  addToHistory("user", escaped);
  addToHistory("assistant", result);
  conversationMode = true;

  Serial.println("Extracted answer: " + result);
  Serial.println("=================================\n");

  return result;
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("   M5StickC Plus Voice Assistant v3.2");
  Serial.println("   Safety + Magic + Respect!");
  Serial.println("========================================\n");

  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);

  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

  Serial.printf("Allocating audio buffer: %d samples, %d bytes\n", 
                RECORD_SAMPLES, RECORD_SAMPLES * sizeof(int16_t));
  audioBuffer = (int16_t*)heap_caps_malloc(RECORD_SAMPLES * sizeof(int16_t), MALLOC_CAP_8BIT);
  if (!audioBuffer) {
    Serial.println("ERROR: Failed to allocate audio buffer!");
    drawScreen("Memory Error!\n\nCan't allocate\naudio buffer\n\nTry restarting");
    while(1) delay(1000);
  }
  Serial.println("Audio buffer allocated successfully");
  Serial.printf("Free heap after buffer: %d bytes\n", ESP.getFreeHeap());

  if (!connectToWiFi()) {
    startAPMode();
    return;
  }

  M5.Imu.update();
  Serial.println("IMU initialized for tilt & shake detection");

  showReadyScreen();
  
  auto mic_cfg = M5.Mic.config();
  mic_cfg.sample_rate = SAMPLE_RATE;
  M5.Mic.config(mic_cfg);
  M5.Mic.begin();
  
  Serial.println("\nReady!");
  Serial.println("Button A = Continue topic");
  Serial.println("Button B = New topic OR hold 3 sec to reset WiFi");
  Serial.println("Tilt <-- (left) = slower");
  Serial.println("Tilt --> (right) = faster");
  Serial.println("Shake = replay last answer");
  Serial.println("Safety guardrails enabled");
  Serial.println("Magic preservation enabled");
  Serial.println("Religious & cultural respect enabled");
  Serial.println();
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  M5.update();

  // Check for low battery warning
  int batteryLevel = M5.Power.getBatteryLevel();
  if (batteryLevel < 15 && batteryLevel > 0) {
    static unsigned long lastBatteryWarning = 0;
    if (millis() - lastBatteryWarning > 60000) { // Show once per minute
      drawScreen("Low Battery!\n\n" + String(batteryLevel) + "%\n\nPlease charge soon");
      delay(2000);
      showReadyScreen();
      lastBatteryWarning = millis();
    }
  }

  // Check for shake to replay
  if (lastResponse.length() > 0 && detectShake()) {
    Serial.println("\n*** SHAKE DETECTED - REPLAYING ***\n");
    
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextDatum(CC_DATUM);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.drawString("Replaying!", WIDTH/2, HEIGHT/2);
    delay(1000);
    
    displayRSVP(lastResponse);
    return;
  }

  if (apMode) {
    server.handleClient();
    if (M5.BtnB.pressedFor(3000)) {
      resetCredentials();
    }
    return;
  }

  if (M5.BtnB.pressedFor(3000)) {
    resetCredentials();
  }

  // BUTTON A: CONTINUE/FIRST QUESTION
  if (M5.BtnA.wasClicked()) {
    
    if (ESP.getFreeHeap() < 40000) {
      drawScreen("Memory low!\nRestarting...");
      delay(2000);
      ESP.restart();
      return;
    }
    
    if (historyCount == 0) {
      Serial.println("\n*** BUTTON A: FIRST QUESTION ***\n");
    } else {
      Serial.println("\n*** BUTTON A: CONTINUE TOPIC ***\n");
    }
    
    Serial.printf("Free heap before recording: %d bytes\n", ESP.getFreeHeap());
    
    if (!recordAudio()) {
      drawScreen("Mic error");
      delay(2000);
      showReadyScreen();
      return;
    }

    Serial.printf("Free heap after recording: %d bytes\n", ESP.getFreeHeap());

    drawTranscribing();
    String question = transcribeAudio();

    if (question.length() < 2 || 
        question.startsWith("No ") || 
        question.startsWith("Parse") || 
        question.startsWith("Connection") || 
        question.startsWith("Timeout") ||
        question.startsWith("WiFi")) {
      Serial.println("Transcription failed or empty");
      drawScreen("Couldn't hear.\nTry again.");
      delay(2000);
      showReadyScreen();
      return;
    }

    drawThinking();
    String answer = askGPT(question);
    
    lastResponse = answer;
    
    displayRSVP(answer);
    
    Serial.printf("Free heap at end: %d bytes\n", ESP.getFreeHeap());
    Serial.println("\n*** CONTINUE TOPIC COMPLETE ***");
    Serial.println("Waiting for user input (A, B, or shake)...\n");
  }

  // BUTTON B (SHORT PRESS): NEW TOPIC
  if (M5.BtnB.wasClicked() && !M5.BtnB.isPressed()) {
    Serial.println("\n*** BUTTON B: NEW TOPIC ***\n");
    clearHistory();
    
    if (ESP.getFreeHeap() < 40000) {
      drawScreen("Memory low!\nRestarting...");
      delay(2000);
      ESP.restart();
      return;
    }
    
    Serial.printf("Free heap before recording: %d bytes\n", ESP.getFreeHeap());
    
    if (!recordAudio()) {
      drawScreen("Mic error");
      delay(2000);
      showReadyScreen();
      return;
    }

    Serial.printf("Free heap after recording: %d bytes\n", ESP.getFreeHeap());

    drawTranscribing();
    String question = transcribeAudio();

    if (question.length() < 2 || 
        question.startsWith("No ") || 
        question.startsWith("Parse") || 
        question.startsWith("Connection") || 
        question.startsWith("Timeout") ||
        question.startsWith("WiFi")) {
      Serial.println("Transcription failed or empty");
      drawScreen("Couldn't hear.\nTry again.");
      delay(2000);
      showReadyScreen();
      return;
    }

    drawThinking();
    String answer = askGPT(question);
    
    lastResponse = answer;
    
    displayRSVP(answer);
    
    Serial.printf("Free heap at end: %d bytes\n", ESP.getFreeHeap());
    Serial.println("\n*** NEW TOPIC COMPLETE ***");
    Serial.println("Waiting for user input (A, B, or shake)...\n");
  }

  delay(20);
}
