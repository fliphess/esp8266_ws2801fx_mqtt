#include <FS.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WS2801FX.h>

#include "content/index.html.h"
#include "content/main.js.h"

// **********************************
// * System settings                *
// **********************************

#define HTTP_PORT 80
#define WIFI_TIMEOUT 30000

const char HOSTNAME[] = "ledstrip1.home";

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// **********************************
// * General LED Strip settings     *
// **********************************

#define LED_COUNT 4
#define LED_DATA_PIN 3
#define LED_CLOCK_PIN 2

WS2801FX ledstrip = WS2801FX(LED_COUNT, LED_DATA_PIN, LED_CLOCK_PIN, WS2801_BGR);

const int STATE_BUFFER = JSON_OBJECT_SIZE(6) + 190;

// **********************************
// * LED Strip Defaults             *
// **********************************

#define DEFAULT_BASE_COLOR 0xFF5900
#define DEFAULT_BASE_BRIGHTNESS 255
#define DEFAULT_BASE_SPEED 200
#define DEFAULT_BASE_MODE FX_MODE_STATIC

#define BRIGHTNESS_STEP 15              // in/decrease brightness by this amount per click
#define SPEED_STEP 10                   // in/decrease brightness by this amount per click

// Will be filled with html for modes listing
String modes = "";


// **********************************
// * MQTT                           *
// **********************************

#define MQTT_MAX_PACKET_SIZE 512
#define MQTT_MAX_RECONNECT_TRIES 10

// To be filled with EEPROM data
char MQTT_HOST[64] = "";
char MQTT_PORT[6]  = "";
char MQTT_USER[32] = "";
char MQTT_PASS[32] = "";

// Reconnect counter
int MQTT_RECONNECT_RETRIES = 0;

// MQTT in and out topic based on hostname
char MQTT_IN_TOPIC[strlen(HOSTNAME) + 4];      // Topic in will be: <HOSTNAME>/in
char MQTT_OUT_TOPIC[strlen(HOSTNAME) + 5];     // Topic out will be: <HOSTNAME>/out

// Buffer for MQTT input stream
const int MQTT_INPUT_BUFFER = JSON_OBJECT_SIZE(6) + 190;

// **********************************
// * Ticker (System LED Blinker)    *
// **********************************

#include <Ticker.h>
Ticker ticker;

// Blink on-board Led
void tick() {
    // Toggle state
    int state = digitalRead(BUILTIN_LED);    // get the current state of GPIO1 pin
    digitalWrite(BUILTIN_LED, !state);       // set pin to the opposite state
}

// **********************************
// * EEPROM and State helpers       *
// **********************************

String readEEPROM(int offset, int len) {
    String res = "";
    for (int i = 0; i < len; ++i) {
        res += char(EEPROM.read(i + offset));
    }
    Serial.printf("readEEPROM(): %s\n", res.c_str());
    return res;
}

void writeEEPROM(int offset, int len, String value) {
    Serial.printf("writeEEPROM(): %s\n", value.c_str());
    for (int i = 0; i < len; ++i) {
        if (i < value.length()) {
            EEPROM.write(i + offset, value[i]);
        } else {
            EEPROM.write(i + offset, NULL);
        }
    }
}

String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length()-1;

    for(int i=0; i<=maxIndex && found<=index; i++){
        if(data.charAt(i)==separator || i==maxIndex){
            found++;
            strIndex[0] = strIndex[1]+1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}


// **************************************
// Callback when config mode is entered *
// **************************************

// Callback notifying us of the need to save config
bool shouldSaveConfig = false;

void saveConfigCallback () {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

// * Gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
    Serial.println(F("Entered config mode"));
    Serial.println(WiFi.softAPIP());

    // If you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());

    // Entered config mode, make led toggle faster
    ticker.attach(0.2, tick);
}


// **********************************
// * Led Controller                 *
// **********************************

// Get the current state of the led controller as a json object
JsonObject& getState() {
    StaticJsonBuffer<STATE_BUFFER> jsonBuffer;
    JsonObject& state = jsonBuffer.createObject();

    state["color"]      = String(ledstrip.getColor(), HEX);
    state["mode"]       = ledstrip.getMode();
    state["speed"]      = ledstrip.getSpeed();
    state["brightness"] = ledstrip.getBrightness();
    state["identity"]   = HOSTNAME;
    return state;
}

// Gets the json object from http or mqtt and parses the color part
void processColor(JsonObject& message) {
    uint32_t new_led_color;
    uint32_t current_led_color;

    current_led_color = ledstrip.getColor();
    new_led_color = current_led_color;

    if (message.containsKey("color")) {
        new_led_color = (uint32_t) strtol(message["color"], NULL, 16);
    }
    else if (message.containsKey("c")) {
        new_led_color = (uint32_t) strtol(message["c"], NULL, 16);
    }

    if (new_led_color != current_led_color) {
        if (new_led_color >= 0x000000 && new_led_color <= 0xFFFFFF) {
            ledstrip.setColor(new_led_color);
        }
    }
}

// Gets the json object from http or mqtt and parses the effect mode
void processMode(JsonObject& message) {
    uint8_t new_led_mode, current_led_mode;
    current_led_mode = ledstrip.getMode();
    new_led_mode = current_led_mode;

    if (message.containsKey("mode")) {
        new_led_mode = (uint8_t) strtol(message["mode"], NULL, 10);
    }
    else if (message.containsKey("m")) {
        new_led_mode = (uint8_t) strtol(message["m"], NULL, 10);
    }

    if (current_led_mode != new_led_mode) {
        ledstrip.setMode(new_led_mode % ledstrip.getModeCount());
    }
}

// Gets the json object from http or mqtt and parses the brightness of the ledstrip
void processBrightness(JsonObject& message) {
    uint8_t current_led_brightness, new_led_brightness;
    current_led_brightness = ledstrip.getBrightness();
    new_led_brightness = current_led_brightness;

    if (message.containsKey("brightness")) {
        new_led_brightness = (uint8_t) strtol(message["brightness"], NULL, 10);
    }
    else if (message.containsKey("b")) {
        new_led_brightness = (uint8_t) strtol(message["b"], NULL, 10);
    }

    if (current_led_brightness != new_led_brightness) {
        ledstrip.setBrightness(new_led_brightness);
    }
}

// Gets the json object from http or mqtt and parses the effect speed
void processSpeed(JsonObject& message) {
    uint8_t current_led_speed, new_led_speed;
    current_led_speed = ledstrip.getSpeed();
    new_led_speed = current_led_speed;

    if (message.containsKey("speed")) {
        new_led_speed = (uint8_t) strtol(message["speed"], NULL, 10);
    }
    else if (message.containsKey("s")) {
        new_led_speed = (uint8_t) strtol(message["s"], NULL, 10);
    }
    if (current_led_speed != new_led_speed) {
        ledstrip.setSpeed(new_led_speed);
    }
}

// Process the incoming json message by dispatching each element to it's own processor function
bool processJson(JsonObject& message) {

    // * Process Color
    if (message.containsKey("color") || message.containsKey("c")) {
        processColor(message);
    }

    // * Process FX Mode
    if (message.containsKey("mode") || message.containsKey("m")) {
        processMode(message);
    }

    // * Process Brightness
    if (message.containsKey("brightness") || message.containsKey("b")) {
        processBrightness(message);
    }

    // * Process Speed
    if (message.containsKey("speed") || message.containsKey("s")) {
        processSpeed(message);
    }

    return true;
}

// Get all effect modes from ws2801 library and create a html object that's included in the frontpage
void setupModes() {
    modes.reserve(5000);
    for(uint8_t i=0; i < ledstrip.getModeCount(); i++) {
        modes += "<li><a href='#' class='m' id='";
        modes += i;
        modes += "'>";
        modes += ledstrip.getModeName(i);
        modes += "</a></li>";
    }
}

// Initiate the ledstrip controller library
void ledControllerSetup() {
    ledstrip.init();
    ledstrip.setMode(DEFAULT_BASE_MODE);
    ledstrip.setColor(DEFAULT_BASE_COLOR);
    ledstrip.setSpeed(DEFAULT_BASE_SPEED);
    ledstrip.setBrightness(DEFAULT_BASE_BRIGHTNESS);
    ledstrip.start();
    Serial.println(F("Led Controller Started"));
}


// **********************************
// * Setup MQTT                     *
// **********************************

// * Send the current state to the MQTT broker
void sendState(bool success) {
    JsonObject& state = getState();
    state["success"] = success;

    char buffer[state.measureLength() + 1];
    state.printTo(buffer, sizeof(buffer));
    mqtt_client.publish(MQTT_OUT_TOPIC, buffer, true);

    Serial.print(F("MQTT: Output: "));
    Serial.print(buffer);
    Serial.println(F(""));
}

// * Callback for incoming MQTT messages
void mqtt_callback(char* topic, byte* payload_in, unsigned int length) {
    uint8_t *payload = (uint8_t *) malloc(length + 1);
    memcpy(payload, payload_in, length);
    payload[length] = NULL;

    StaticJsonBuffer<MQTT_INPUT_BUFFER> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(payload);

    if (root.success()) {
        bool result = processJson(root);
        if (result) {
            sendState(result);
        }
    }
    else {
        Serial.println(F("Failed to parse json input from MQTT broker: parseObject() failed."));
    }

    free(payload);
}

// Reconnect to MQTT server and subscribe to in and out topics
void mqtt_reconnect() {
    // * Loop until we're reconnected
    while (!mqtt_client.connected() && MQTT_RECONNECT_RETRIES < MQTT_MAX_RECONNECT_TRIES) {
        MQTT_RECONNECT_RETRIES++;

        Serial.printf("Attempting MQTT connection %d / %d ...\n", MQTT_RECONNECT_RETRIES, MQTT_MAX_RECONNECT_TRIES);

        // * Attempt to connect
        if (mqtt_client.connect(HOSTNAME, MQTT_USER, MQTT_PASS)) {
            Serial.println(F("MQTT connected!"));

            // Once connected, publish an announcement...
            char * message = new char[23 + strlen(HOSTNAME) + 1];
            strcpy(message, "ws2801 ledstrip alive: ");
            strcat(message, HOSTNAME);
            mqtt_client.publish("hass/status", message);

            // * ... and resubscribe
            mqtt_client.subscribe(MQTT_IN_TOPIC);

            Serial.printf("MQTT topic in: %s\n", MQTT_IN_TOPIC);
            Serial.printf("MQTT topic out: %s\n", MQTT_OUT_TOPIC);

        } else {
            Serial.println(F("failed, rc="));
            Serial.println(mqtt_client.state());
            Serial.println(F(" Retrying in 5 seconds"));
            Serial.println("");

            // * Wait 5 seconds before retrying
            delay(5000);
        }
    }

    if (MQTT_RECONNECT_RETRIES >= MQTT_MAX_RECONNECT_TRIES) {
        Serial.printf("*** MQTT connection failed, giving up after %d tries ...\n", MQTT_RECONNECT_RETRIES);
    }
}


// **********************************
// * Setup Webserver                *
// **********************************

ESP8266WebServer server = ESP8266WebServer(HTTP_PORT);

void srv_handle_not_found() {
    // Handle 404 not found error
    server.send(404, "text/plain", "File Not Found");
}

void srv_handle_index_html() {
    // Send the main index file
    server.send_P(200,"text/html", index_html);
}

void srv_handle_main_js() {
    // Send "our" javascript code
    server.send_P(200,"application/javascript", main_js);
}

void srv_handle_modes() {
    // Return a table with all available modes
    server.send(200,"text/plain", modes);
}

void srv_handle_json() {
    StaticJsonBuffer<600> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));

    if ( root.success() ) {
        bool result = processJson(root);
        if (result) {
            sendState(result);
            server.send(200, "application/json", "{\"success\":\"true\",\"message\":\"OK\"}");
        }
    }
    else {
        Serial.println(F("Failed to parse json input from http request: parseObject() failed."));
        server.send(500, "application/json", "{\"success\":\"false\",\"message\":\"Update Failed\"}");
    }

}

void srv_handle_status() {
    /*
     * Handle the status endpoint
     */
    JsonObject& status = getState();

    char buffer[status.measureLength() + 1];
    status.printTo(buffer, sizeof(buffer));

    server.send(200, F("application/json"), buffer);
}

void srv_handle_set() {
    for (uint8_t argument=0; argument < server.args(); argument++){

        // * Process Color
        if(server.argName(argument) == "c" or server.argName(argument) == "colour") {
            uint32_t color = (uint32_t) strtol(&server.arg(argument)[0], NULL, 16);
            if(color >= 0x000000 && color <= 0xFFFFFF) {
                ledstrip.setColor(color);
            }
        }

        // * Process FX Mode
        if(server.argName(argument) == "m" or server.argName(argument) == "mode") {
            uint8_t led_mode = (uint8_t) strtol(&server.arg(argument)[0], NULL, 10);
            ledstrip.setMode(led_mode % ledstrip.getModeCount());
        }

        // * Process Brightness
        if(server.argName(argument) == "b" or server.argName(argument) == "brightness") {
            if(server.arg(argument)[0] == '-') {
                ledstrip.decreaseBrightness(BRIGHTNESS_STEP);
            } else {
                ledstrip.increaseBrightness(BRIGHTNESS_STEP);
            }
        }

        // * Process Speed
        if(server.argName(argument) == "s" or server.argName(argument) == "speed") {
            if(server.arg(argument)[0] == '-') {
                ledstrip.decreaseSpeed(SPEED_STEP);
            } else {
                ledstrip.increaseSpeed(SPEED_STEP);
            }
        }
    }
    sendState(true);
    server.send(200, "text/plain", "OK");
}

void webserverSetup() {
    Serial.println(F("Setup HTTP Webserver"));

    server.on("/", srv_handle_index_html);
    server.on("/main.js", srv_handle_main_js);
    server.on("/modes", srv_handle_modes);
    server.on("/set", srv_handle_set);
    server.on("/status", srv_handle_status);
    server.on("/json", srv_handle_json);

    server.onNotFound(srv_handle_not_found);
    server.begin();

    Serial.println(F("HTTP Webserver Started"));
}


// **********************************
// Setup Main                       *
// **********************************

void setup(){

    // * Configure Serial and EEPROM
    Serial.begin(9600);
    EEPROM.begin(512);

    // * Form SSID String
    String ssid = "ESP" + String(ESP.getChipId());

    // * Set led pin as output
    pinMode(BUILTIN_LED, OUTPUT);

    // * Start ticker with 0.5 because we start in AP mode and try to connect
    ticker.attach(0.6, tick);

    // Get MQTT Server settings
    String settings_available = readEEPROM(134, 1);

    if (settings_available == "1") {
        readEEPROM(0, 64).toCharArray(MQTT_HOST, 64);   // 0-63
        readEEPROM(64, 6).toCharArray(MQTT_PORT, 6);    // 64-69
        readEEPROM(70, 32).toCharArray(MQTT_USER, 32);  // 70-101
        readEEPROM(102, 32).toCharArray(MQTT_PASS, 32); // 102-133

        Serial.printf("MQTT host: %s\n", MQTT_HOST);
        Serial.printf("MQTT port: %s\n", MQTT_PORT);
        Serial.printf("MQTT user: %s\n", MQTT_USER);
        Serial.printf("MQTT pass: %s\n", MQTT_PASS);
    }

    WiFiManagerParameter CUSTOM_MQTT_HOST("host", "MQTT hostname", MQTT_HOST, 64);
    WiFiManagerParameter CUSTOM_MQTT_PORT("port", "MQTT port", MQTT_PORT, 6);
    WiFiManagerParameter CUSTOM_MQTT_USER("user", "MQTT user", MQTT_USER, 32);
    WiFiManagerParameter CUSTOM_MQTT_PASS("pass", "MQTT pass", MQTT_PASS, 32);

    // * WiFiManager local initialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // * Reset settings - uncomment for testing
    // wifiManager.resetSettings();

    // * Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(configModeCallback);

    // * Set timeout
    wifiManager.setConfigPortalTimeout(WIFI_TIMEOUT);

    // * Set save config callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    // Add all your parameters here
    wifiManager.addParameter(&CUSTOM_MQTT_HOST);
    wifiManager.addParameter(&CUSTOM_MQTT_PORT);
    wifiManager.addParameter(&CUSTOM_MQTT_USER);
    wifiManager.addParameter(&CUSTOM_MQTT_PASS);

    // * Fetches SSID and pass and tries to connect
    // * Reset when no connection after 10 seconds
    if (!wifiManager.autoConnect()) {
        Serial.println(F("Failed to connect and hit timeout"));

        // Reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(WIFI_TIMEOUT);
    }

    // Read updated parameters
    strcpy(MQTT_HOST, CUSTOM_MQTT_HOST.getValue());
    strcpy(MQTT_PORT, CUSTOM_MQTT_PORT.getValue());
    strcpy(MQTT_USER, CUSTOM_MQTT_USER.getValue());
    strcpy(MQTT_PASS, CUSTOM_MQTT_PASS.getValue());

    // Save the custom parameters to FS
    if (shouldSaveConfig) {
        Serial.println("Saving WiFiManager config");

        writeEEPROM(0, 64, MQTT_HOST);   // 0-63
        writeEEPROM(64, 6, MQTT_PORT);   // 64-69
        writeEEPROM(70, 32, MQTT_USER);  // 70-101
        writeEEPROM(102, 32, MQTT_PASS); // 102-133
        writeEEPROM(134, 1, "1");        // 134 --> always "1"
        EEPROM.commit();
    }

    // * If you get here you have connected to the WiFi
    Serial.println(F("Connected to WIFI..."));

    // * Keep LED on
    ticker.detach();
    digitalWrite(BUILTIN_LED, LOW);

    // * Configure Controller Modes
    Serial.println(F("Setup Controller Modes"));
    setupModes();

    // * Configure LED Controller
    Serial.println(F("Setup Led Controller"));
    ledControllerSetup();

    // * Setup Webserver
    Serial.println(F("HTTP server setup"));
    webserverSetup();

    // * Configure MQTT
    Serial.println(F("Setup MQTT"));

    if (MQTT_HOST != "" && MQTT_PORT != "" && MQTT_USER != "" && MQTT_PASS != "") {
        snprintf(MQTT_IN_TOPIC, sizeof MQTT_IN_TOPIC, "%s/in", HOSTNAME);
        snprintf(MQTT_OUT_TOPIC, sizeof MQTT_OUT_TOPIC, "%s/out", HOSTNAME);

        Serial.printf("MQTT active: %s:%d\n", MQTT_HOST, String(MQTT_PORT).toInt());
        mqtt_client.setServer(MQTT_HOST, String(MQTT_PORT).toInt());
        mqtt_client.setCallback(mqtt_callback);
    }

    Serial.println(F("Setup Done!"));
}


// **********************************
// * Loop                           *
// **********************************

void loop() {
    server.handleClient();
    ledstrip.service();

    if (MQTT_RECONNECT_RETRIES < MQTT_MAX_RECONNECT_TRIES) {
        if (!mqtt_client.connected()) {
            mqtt_reconnect();
        } else {
            mqtt_client.loop();
        }
    }
}
