#include <DHT.h>

// ==================== PIN DEFINITIONS ====================
#define SOIL_MOISTURE_PIN 34      // GPIO34 (ADC) - Analog input for soil moisture sensor
#define DHT_PIN 4                 // GPIO4 - DHT11 data pin
#define RELAY_PIN 25              // GPIO25 - Controls the water pump relay module

// ==================== DHT11 SENSOR ====================
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// ==================== CONFIGURATION ====================
const int MOISTURE_THRESHOLD = 2000;  // Threshold value for triggering pump (0-4095)
const int MOISTURE_HYSTERESIS = 200;  // Hysteresis to prevent pump chatter
const unsigned long SENSOR_READ_INTERVAL = 5000;  // Read sensors every 5 seconds
const unsigned long PUMP_RUN_TIME = 5000;         // Run pump for 5 seconds
const unsigned long PUMP_COOLDOWN = 10000;        // Wait 10 seconds before next pump run

// ==================== VARIABLES ====================
unsigned long lastSensorRead = 0;
unsigned long pumpStartTime = 0;
unsigned long lastPumpTime = 0;
bool pumpActive = false;
bool pumpCoolingDown = false;

int soilMoisture = 0;
float temperature = 0;
float humidity = 0;

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== IoT Plant Watering System ===");
  Serial.println("Initializing system...");
  
  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Ensure pump is OFF at startup
  
  // Initialize DHT11
  dht.begin();
  Serial.println("✓ DHT11 initialized");
  Serial.println("✓ System ready!\n");
  
  printHeader();
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long currentTime = millis();
  
  // Read sensors at specified interval
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
    readSensors();
    lastSensorRead = currentTime;
  }
  
  // Control pump based on moisture level
  controlPump(currentTime);
}

// ==================== PRINT HEADER ====================
void printHeader() {
  Serial.println("===================================================");
  Serial.println("Time(ms) | Moisture(%) | Temp(°C) | Humidity(%) | Pump Status");
  Serial.println("===================================================");
}

// ==================== READ SENSORS ====================
void readSensors() {
  // Read soil moisture (raw ADC value: 0-4095)
  int rawMoisture = analogRead(SOIL_MOISTURE_PIN);
  
  // Convert raw value to percentage
  // Lower ADC = More moisture (sensor submerged)
  // Higher ADC = Less moisture (sensor dry)
  soilMoisture = map(rawMoisture, 4095, 0, 0, 100);  // Invert mapping
  
  // Read DHT11 temperature and humidity
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  // Validate readings
  if (!isnan(temp)) {
    temperature = temp;
  }
  if (!isnan(hum)) {
    humidity = hum;
  }
  
  // Print formatted sensor data
  printSensorData();
}

// ==================== PRINT SENSOR DATA ====================
void printSensorData() {
  Serial.print(millis());
  Serial.print(" | ");
  Serial.print(soilMoisture);
  Serial.print("% | ");
  Serial.print(temperature, 1);
  Serial.print("°C | ");
  Serial.print(humidity, 1);
  Serial.print("% | ");
  
  if (pumpActive) {
    Serial.println("[RUNNING]");
  } else if (pumpCoolingDown) {
    Serial.println("[COOLDOWN]");
  } else {
    Serial.println("[IDLE]");
  }
}

// ==================== PUMP CONTROL LOGIC ====================
void controlPump(unsigned long currentTime) {
  // Check if pump is currently running
  if (pumpActive) {
    // Check if pump run time has elapsed
    if (currentTime - pumpStartTime >= PUMP_RUN_TIME) {
      stopPump(currentTime);
    }
  }
  // Check if pump is on cooldown
  else if (pumpCoolingDown) {
    // Check if cooldown period has elapsed
    if (currentTime - lastPumpTime >= PUMP_COOLDOWN) {
      pumpCoolingDown = false;
      Serial.println("→ Pump cooldown complete");
    }
  }
  // Normal operation: check moisture level
  else {
    // Trigger pump if moisture is too low and not on cooldown
    if (soilMoisture < MOISTURE_THRESHOLD) {
      startPump(currentTime);
      Serial.println("→ Moisture low: Pump activated");
    }
  }
}

// ==================== START PUMP ====================
void startPump(unsigned long currentTime) {
  if (!pumpActive) {
    digitalWrite(RELAY_PIN, HIGH);  // Activate relay (pump ON)
    pumpActive = true;
    pumpStartTime = currentTime;
    Serial.println("[PUMP ON] Water pump activated");
  }
}

// ==================== STOP PUMP ====================
void stopPump(unsigned long currentTime) {
  if (pumpActive) {
    digitalWrite(RELAY_PIN, LOW);   // Deactivate relay (pump OFF)
    pumpActive = false;
    lastPumpTime = currentTime;
    pumpCoolingDown = true;
    Serial.println("[PUMP OFF] Water pump deactivated - Entering cooldown");
  }
}
