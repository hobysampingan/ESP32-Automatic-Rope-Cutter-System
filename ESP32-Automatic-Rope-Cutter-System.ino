#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi Credentials
const char* ssid = "WIFI_SSID_ANDA";
const char* password = "PASSWORD_ANDA";

// Pin Configuration ESP32
// BTS7960 Motor Driver
#define MOTOR_RPWM 25       // Right PWM (Forward)
#define MOTOR_LPWM 26       // Left PWM (Reverse)
#define MOTOR_R_EN 27       // Right Enable
#define MOTOR_L_EN 14       // Left Enable
#define BUTTON_PIN 32       // Pin push button untuk start
#define LED_PIN 2           // LED indicator built-in ESP32
#define OLED_SDA 21         // OLED SDA (default I2C)
#define OLED_SCL 22         // OLED SCL (default I2C)

// Motor speed (0-255)
#define MOTOR_SPEED 200     // Adjust sesuai kebutuhan (max 255)

// OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Web Server
WebServer server(80);

// Global Variables
float targetLength = 0.0;        // Target panjang tali (meter)
float calibMeterPerRot = 0.05;   // Kalibrasi: meter per putaran
float currentLength = 0.0;       // Panjang tali yang sudah keluar
bool motorRunning = false;
unsigned long motorStartTime = 0;
unsigned long lastButtonPress = 0;
bool buttonPressed = false;

// HTML Web Interface (EMBEDDED)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Kontrol Pemotong Tali ESP32</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        .header h1 {
            font-size: 28px;
            margin-bottom: 10px;
        }
        .status {
            display: inline-block;
            padding: 8px 20px;
            border-radius: 20px;
            background: rgba(76, 175, 80, 0.3);
            font-size: 14px;
            margin-top: 10px;
        }
        .content {
            padding: 30px;
        }
        .section {
            margin-bottom: 30px;
            padding: 25px;
            background: #f8f9fa;
            border-radius: 15px;
            border: 2px solid #e9ecef;
        }
        .section h2 {
            color: #667eea;
            margin-bottom: 20px;
            font-size: 20px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        .form-group label {
            display: block;
            margin-bottom: 8px;
            color: #495057;
            font-weight: 600;
            font-size: 14px;
        }
        .form-group input {
            width: 100%;
            padding: 12px 15px;
            border: 2px solid #dee2e6;
            border-radius: 10px;
            font-size: 16px;
            transition: all 0.3s;
        }
        .form-group input:focus {
            outline: none;
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        .button {
            padding: 15px 30px;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        .button-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            width: 100%;
        }
        .button-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 25px rgba(102, 126, 234, 0.3);
        }
        .button-secondary {
            background: #6c757d;
            color: white;
            margin-right: 10px;
        }
        .button-success {
            background: #28a745;
            color: white;
        }
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-top: 15px;
        }
        .info-card {
            background: white;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            border: 2px solid #e9ecef;
        }
        .info-card .value {
            font-size: 32px;
            font-weight: bold;
            color: #667eea;
            margin: 10px 0;
        }
        .info-card .label {
            font-size: 14px;
            color: #6c757d;
            text-transform: uppercase;
        }
        .alert {
            padding: 15px 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            font-size: 14px;
        }
        .alert-info {
            background: #d1ecf1;
            color: #0c5460;
            border: 1px solid #bee5eb;
        }
        .alert-warning {
            background: #fff3cd;
            color: #856404;
            border: 1px solid #ffeaa7;
        }
        .alert-success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .ip-box {
            background: white;
            padding: 15px;
            border-radius: 10px;
            text-align: center;
            margin-top: 15px;
            border: 2px solid #667eea;
        }
        .ip-box .ip {
            font-size: 24px;
            font-weight: bold;
            color: #667eea;
            font-family: monospace;
        }
        .chip-badge {
            display: inline-block;
            background: rgba(255,255,255,0.2);
            padding: 5px 15px;
            border-radius: 15px;
            font-size: 12px;
            margin-top: 10px;
        }
        .button-group {
            display: flex;
            gap: 10px;
            flex-wrap: wrap;
        }
        @media (max-width: 600px) {
            .button-group {
                flex-direction: column;
            }
            .button {
                width: 100%;
                margin: 0 0 10px 0 !important;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔧 Kontrol Pemotong Tali</h1>
            <div class="chip-badge">⚡ ESP32 Dual Core</div>
            <div class="status">● Ready with OLED Display</div>
            <div class="ip-box">
                <div style="font-size: 14px; color: rgba(255,255,255,0.8); margin-bottom: 5px;">IP Address:</div>
                <div class="ip" id="ipAddress">Loading...</div>
            </div>
        </div>
        
        <div class="content">
            <div class="alert alert-success">
                📺 Lihat progress real-time di layar OLED ESP32!
            </div>

            <!-- Setting Panjang Tali -->
            <div class="section">
                <h2>📏 Setting Panjang Tali</h2>
                <div class="form-group">
                    <label for="targetLength">Panjang Tali (meter)</label>
                    <input type="number" id="targetLength" placeholder="Masukkan panjang dalam meter" step="0.1" min="0.1">
                </div>
                <button class="button button-primary" onclick="setTarget()">
                    Set Panjang Target
                </button>
            </div>
            
            <!-- Kalibrasi Motor -->
            <div class="section">
                <h2>⚙️ Kalibrasi Motor</h2>
                <div class="alert alert-info">
                    Ukur berapa meter tali yang keluar dalam 1 putaran motor penuh atau dalam waktu tertentu
                </div>
                <div class="form-group">
                    <label for="calibLength">Panjang Aktual (meter)</label>
                    <input type="number" id="calibLength" placeholder="Contoh: 0.5" step="0.01" min="0.01">
                </div>
                <div class="form-group">
                    <label for="calibRotation">Jumlah Putaran/Waktu</label>
                    <input type="number" id="calibRotation" placeholder="Contoh: 10" step="1" min="1">
                </div>
                <div class="button-group">
                    <button class="button button-secondary" onclick="testMotor()">
                        Test Motor (10 detik)
                    </button>
                    <button class="button button-success" onclick="saveCalibration()">
                        Simpan Kalibrasi
                    </button>
                </div>
            </div>
            
            <!-- Info Status -->
            <div class="section">
                <h2>📊 Status & Info</h2>
                <div class="info-grid">
                    <div class="info-card">
                        <div class="label">Target</div>
                        <div class="value" id="displayTarget">0.0</div>
                        <div class="label">meter</div>
                    </div>
                    <div class="info-card">
                        <div class="label">Kalibrasi</div>
                        <div class="value" id="displayCalib">0.0</div>
                        <div class="label">m/putaran</div>
                    </div>
                    <div class="info-card">
                        <div class="label">Estimasi Waktu</div>
                        <div class="value" id="displayTime">0</div>
                        <div class="label">detik</div>
                    </div>
                </div>
            </div>
            
            <!-- Kontrol Manual -->
            <div class="section">
                <h2>🎮 Kontrol Manual</h2>
                <div class="alert alert-warning">
                    Gunakan tombol fisik pada perangkat untuk memulai proses pemotongan
                </div>
                <div class="button-group">
                    <button class="button button-secondary" onclick="stopMotor()">
                        ⏹️ Stop Motor
                    </button>
                    <button class="button button-secondary" onclick="resetCounter()">
                        🔄 Reset Counter
                    </button>
                </div>
            </div>
        </div>
    </div>

    <script>
        let config = {
            targetLength: 0,
            calibMeterPerRot: 0.05,
            calibRotations: 10
        };

        function loadFromMemory() {
            const saved = localStorage.getItem('ropeConfig');
            if (saved) {
                config = JSON.parse(saved);
            }
            updateDisplay();
        }

        function saveToMemory() {
            localStorage.setItem('ropeConfig', JSON.stringify(config));
        }

        function updateDisplay() {
            document.getElementById('displayTarget').textContent = config.targetLength.toFixed(1);
            document.getElementById('displayCalib').textContent = config.calibMeterPerRot.toFixed(3);
            const estTime = config.targetLength > 0 ? (config.targetLength / config.calibMeterPerRot * 2).toFixed(0) : 0;
            document.getElementById('displayTime').textContent = estTime;
        }

        function setTarget() {
            const length = parseFloat(document.getElementById('targetLength').value);
            if (isNaN(length) || length <= 0) {
                alert('❌ Masukkan panjang yang valid!');
                return;
            }
            config.targetLength = length;
            saveToMemory();
            updateDisplay();
            sendToESP({cmd: 'setTarget', value: length});
            alert('✅ Target diset: ' + length + ' meter');
        }

        function testMotor() {
            if (confirm('⚠️ Motor akan berjalan selama 10 detik. Siap?')) {
                sendToESP({cmd: 'testMotor', duration: 10000});
                alert('▶️ Motor berjalan... Ukur panjang tali yang keluar!');
            }
        }

        function saveCalibration() {
            const length = parseFloat(document.getElementById('calibLength').value);
            const rotations = parseFloat(document.getElementById('calibRotation').value);
            
            if (isNaN(length) || isNaN(rotations) || length <= 0 || rotations <= 0) {
                alert('❌ Masukkan nilai kalibrasi yang valid!');
                return;
            }
            
            config.calibMeterPerRot = length / rotations;
            config.calibRotations = rotations;
            saveToMemory();
            updateDisplay();
            sendToESP({cmd: 'setCalib', meterPerRot: config.calibMeterPerRot});
            alert('✅ Kalibrasi disimpan: ' + config.calibMeterPerRot.toFixed(3) + ' m/putaran');
        }

        function stopMotor() {
            sendToESP({cmd: 'stop'});
            alert('⏹️ Perintah stop dikirim!');
        }

        function resetCounter() {
            if (confirm('🔄 Reset counter?')) {
                sendToESP({cmd: 'reset'});
                alert('✅ Counter direset!');
            }
        }

        function sendToESP(data) {
            fetch('/api', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(data)
            }).catch(err => console.error('Error:', err));
        }

        fetch('/ip').then(r => r.text()).then(ip => {
            document.getElementById('ipAddress').textContent = ip;
        });

        loadFromMemory();
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // Setup Pins untuk BTS7960
  pinMode(MOTOR_RPWM, OUTPUT);
  pinMode(MOTOR_LPWM, OUTPUT);
  pinMode(MOTOR_R_EN, OUTPUT);
  pinMode(MOTOR_L_EN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  // Setup PWM untuk motor control yang lebih smooth
  // Untuk ESP32 Core 3.0+: gunakan ledcAttach
  ledcAttach(MOTOR_RPWM, 5000, 8); // Pin, freq 5kHz, 8-bit resolution
  ledcAttach(MOTOR_LPWM, 5000, 8); // Pin, freq 5kHz, 8-bit resolution
  
  // Initialize motor - STOP
  digitalWrite(MOTOR_R_EN, HIGH); // Enable right side
  digitalWrite(MOTOR_L_EN, HIGH); // Enable left side
  ledcWrite(MOTOR_RPWM, 0); // RPWM = 0
  ledcWrite(MOTOR_LPWM, 0); // LPWM = 0
  digitalWrite(LED_PIN, LOW);
  
  // Initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    // Lanjutkan tanpa OLED jika tidak terdeteksi
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("Rope Cutter");
    display.println("ESP32 Edition");
    display.println("");
    display.println("Connecting WiFi...");
    display.display();
  }
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    attempt++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n\n========================================");
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("========================================");
    Serial.println("Buka browser dan ketik IP di atas");
    Serial.println("========================================\n");
    digitalWrite(LED_PIN, HIGH);
    
    // Update OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("WiFi Connected!");
    display.println("");
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.println("");
    display.println("Ready to use!");
    display.display();
  } else {
    Serial.println("\nWiFi connection failed!");
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("WiFi Failed!");
    display.println("Check credentials");
    display.display();
  }
  
  // Setup Web Server Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/ip", HTTP_GET, handleIP);
  server.on("/api", HTTP_POST, handleAPI);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  // Check push button (dengan debouncing)
  if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
    if (millis() - lastButtonPress > 300) {
      buttonPressed = true;
      lastButtonPress = millis();
      handleButtonPress();
    }
  } else if (digitalRead(BUTTON_PIN) == HIGH) {
    buttonPressed = false;
  }
  
  // Motor control logic
  if (motorRunning) {
    updateMotorProgress();
    updateOLED();
    
    if (currentLength >= targetLength) {
      stopMotor();
      Serial.println("Target reached!");
      blinkSuccess();
    }
  }
}

void handleButtonPress() {
  Serial.println("Button pressed!");
  
  if (targetLength <= 0) {
    Serial.println("Error: Target length not set!");
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,20);
    display.println("ERROR!");
    display.setTextSize(1);
    display.setCursor(0,40);
    display.println("Set target first");
    display.display();
    blinkError();
    delay(2000);
    return;
  }
  
  if (motorRunning) {
    stopMotor();
  } else {
    startMotor();
  }
}

void startMotor() {
  motorRunning = true;
  currentLength = 0.0;
  motorStartTime = millis();
  
  // BTS7960: Forward direction (RPWM active, LPWM off)
  ledcWrite(MOTOR_RPWM, MOTOR_SPEED); // RPWM = speed
  ledcWrite(MOTOR_LPWM, 0);            // LPWM = 0
  
  digitalWrite(LED_PIN, HIGH);
  Serial.println("Motor started");
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20,20);
  display.println("RUNNING");
  display.display();
}

void stopMotor() {
  motorRunning = false;
  
  // BTS7960: Stop motor
  ledcWrite(MOTOR_RPWM, 0); // RPWM = 0
  ledcWrite(MOTOR_LPWM, 0); // LPWM = 0
  
  digitalWrite(LED_PIN, LOW);
  Serial.println("Motor stopped");
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20,10);
  display.println("STOPPED");
  display.setTextSize(1);
  display.setCursor(0,35);
  display.print("Cut: ");
  display.print(currentLength, 2);
  display.println("m");
  display.display();
}

void updateMotorProgress() {
  unsigned long elapsedTime = millis() - motorStartTime;
  float estimatedRotations = (elapsedTime / 1000.0) * 1.67;
  currentLength = estimatedRotations * calibMeterPerRot;
  
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    Serial.printf("Progress: %.2f / %.2f meter (%.0f%%)\n", 
                  currentLength, targetLength, 
                  (currentLength/targetLength)*100);
  }
}

void updateOLED() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 200) {
    lastUpdate = millis();
    
    display.clearDisplay();
    display.setTextSize(1);
    
    // Title
    display.setCursor(0,0);
    display.println("CUTTING...");
    
    // Progress bar
    int barWidth = 120;
    int barHeight = 10;
    int progress = (currentLength / targetLength) * barWidth;
    display.drawRect(4, 15, barWidth, barHeight, SSD1306_WHITE);
    display.fillRect(4, 15, progress, barHeight, SSD1306_WHITE);
    
    // Percentage
    display.setTextSize(2);
    display.setCursor(40, 30);
    display.print((int)((currentLength/targetLength)*100));
    display.println("%");
    
    // Current / Target
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print(currentLength, 2);
    display.print(" / ");
    display.print(targetLength, 1);
    display.println(" m");
    
    display.display();
  }
}

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleIP() {
  server.send(200, "text/plain", WiFi.localIP().toString());
}

void handleAPI() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }
  
  String body = server.arg("plain");
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  String cmd = doc["cmd"];
  
  if (cmd == "setTarget") {
    targetLength = doc["value"];
    Serial.printf("Target set to: %.2f meters\n", targetLength);
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,20);
    display.println("Target Set:");
    display.setTextSize(2);
    display.setCursor(20,35);
    display.print(targetLength, 1);
    display.println(" m");
    display.display();
    delay(1500);
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  }
  else if (cmd == "setCalib") {
    calibMeterPerRot = doc["meterPerRot"];
    Serial.printf("Calibration: %.4f m/rotation\n", calibMeterPerRot);
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  }
  else if (cmd == "testMotor") {
    int duration = doc["duration"];
    Serial.printf("Test motor for %d ms\n", duration);
    
    // BTS7960: Run forward
    ledcWrite(MOTOR_RPWM, MOTOR_SPEED);
    ledcWrite(MOTOR_LPWM, 0);
    digitalWrite(LED_PIN, HIGH);
    delay(duration);
    
    // Stop
    ledcWrite(MOTOR_RPWM, 0);
    ledcWrite(MOTOR_LPWM, 0);
    digitalWrite(LED_PIN, LOW);
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  }
  else if (cmd == "stop") {
    stopMotor();
    server.send(200, "application/json", "{\"status\":\"stopped\"}");
  }
  else if (cmd == "reset") {
    currentLength = 0.0;
    stopMotor();
    server.send(200, "application/json", "{\"status\":\"reset\"}");
  }
  else {
    server.send(400, "application/json", "{\"error\":\"Unknown command\"}");
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void blinkSuccess() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}

void blinkError() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(300);
    digitalWrite(LED_PIN, LOW);
    delay(300);
  }
}
