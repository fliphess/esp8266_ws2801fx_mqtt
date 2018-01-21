#include <FS.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <WiFiManager.h>
#include <WS2801FX.h>
#include <ESP8266mDNS.h>

// * Include settings
#include "settings.h"

#include <PubSubClient.h>

// * Include html content constants
#include "content/index.html.h"
#include "content/main.js.h"

// * Initiate led blinker library
Ticker ticker;

// * Initiate HTTP server
ESP8266WebServer webserver = ESP8266WebServer(HTTP_PORT);

// * Initiate Led driver
WS2801FX ledstrip = WS2801FX(LED_COUNT, LED_DATA_PIN, LED_CLOCK_PIN, LED_PIXEL_ORDER);

// * Initiate WIFI client
WiFiClient espClient;

// * Initiate MQTT client
PubSubClient mqtt_client(espClient);


// **********************************
// * WIFI                           *
// **********************************

// * Gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager) {

    #ifdef DEBUG_PRINT
    Serial.println(F("Entered config mode"));
    Serial.println(WiFi.softAPIP());

    // * If you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());
    #endif

    // * Entered config mode, make led toggle faster
    ticker.attach(0.2, tick);
}


// **********************************
// * Ticker (System LED Blinker)    *
// **********************************

// * Blink on-board Led
void tick() {
    // * Toggle state
    int state = digitalRead(BUILTIN_LED);    // * Get the current state of GPIO1 pin
    digitalWrite(BUILTIN_LED, !state);       // * Set pin to the opposite state
}


// **********************************
// * Led Controller                 *
// **********************************

// * Get the current ON or OFF state of the ledstrip
const char* get_on_or_off_state() {
    // * If either stopped or one in brightness, color or mode are zero, return OFF
    return ((!ledstrip.isRunning()) || ledstrip.getMode() == uint8_t(0) || ledstrip.getBrightness() == uint8_t(0) || ledstrip.getColor() == uint32_t(0)) ? STATE_OFF : STATE_ON;
}

// * Update the current state of the led controller as a json object
void update_json_output_buffer() {
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    JsonObject& color = root.createNestedObject("color");

    uint32_t hexcolor = ledstrip.getColor();
    color["r"] = ((hexcolor >> 16) & 0xFF);
    color["g"] = ((hexcolor >> 8) & 0xFF);
    color["b"] = (hexcolor & 0xFF);

    root["effect"]     = ledstrip.getMode();
    root["brightness"] = ledstrip.getBrightness();
    root["speed"]      = ledstrip.getSpeed();
    root["state"]      = get_on_or_off_state();

    root.printTo(LEDSTRIP_JSON_OUTPUT, sizeof(LEDSTRIP_JSON_OUTPUT));
}

// * Process the incoming json message by dispatching each element to it's own library function
bool process_json_input(char* payload) {

    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(payload);

    if (root.success()) {

        // * Process State
        if (root.containsKey("state")) {
            if (root["state"] ==  "OFF") {
                ledstrip.stop();

                // * Do not process anything else when turned OFF
                return true;
            }
            else if (root["state"] == "ON") {
                ledstrip.start();

                if (ledstrip.getMode() == uint8_t(0) && !root.containsKey("effect")) {
                    ledstrip.setMode(DEFAULT_BASE_MODE);
                }
            }
        }

        // * Process Color
        if (root.containsKey("color")) {
            uint8_t red   = root["color"]["r"];
            uint8_t green = root["color"]["g"];
            uint8_t blue  = root["color"]["b"];

            ledstrip.setColor(red, green, blue);
        }

        // * Process FX Mode
        if (root.containsKey("effect")) {
            uint8_t new_led_mode, current_led_mode;

            current_led_mode = ledstrip.getMode();
            new_led_mode = (uint8_t) strtol(root["effect"], NULL, 10);

            if (current_led_mode != new_led_mode) {
                ledstrip.setMode(new_led_mode % ledstrip.getModeCount());
            }
        }

        // * Process Brightness
        if (root.containsKey("brightness")) {
            uint8_t current_led_brightness, new_led_brightness;

            current_led_brightness = ledstrip.getBrightness();
            new_led_brightness = (uint8_t) strtol(root["brightness"], NULL, 10);

            if (new_led_brightness >= uint8_t(0) || new_led_brightness <= uint8_t(255)) {
                if (current_led_brightness != new_led_brightness) {
                    ledstrip.setBrightness(new_led_brightness);
                }
            }
        }

        // * Process Speed
        if (root.containsKey("speed")) {
            uint8_t current_led_speed, new_led_speed;

            current_led_speed = ledstrip.getSpeed();
            new_led_speed = (uint8_t) strtol(root["speed"], NULL, 10);

            if (new_led_speed >= uint8_t(0) || new_led_speed <= uint8_t(255)) {
                if (current_led_speed != new_led_speed) {
                    ledstrip.setSpeed(new_led_speed);
                }
            }
        }

        return true;
    }

    return false;
}

// * Initiate the ledstrip controller library
void setup_led_controller() {
    ledstrip.init();
    ledstrip.setMode(DEFAULT_BASE_MODE);
    ledstrip.setColor(DEFAULT_BASE_COLOR);
    ledstrip.setSpeed(DEFAULT_BASE_SPEED);
    ledstrip.setBrightness(DEFAULT_BASE_BRIGHTNESS);
    ledstrip.start();

    #ifdef DEBUG_PRINT
    Serial.println(F("Led Controller Started"));
    #endif
}


// **********************************
// * Webserver functions            *
// **********************************

// * The main web page
void webserver_handle_index_html() {
    // * Send the main index file
    webserver.send_P(200, "text/html", index_html);
}

// * Javascript endpoint
void webserver_handle_main_js() {
    // * Send "our" javascript code
    webserver.send_P(200, "application/javascript", main_js);
}

// * Send a html table with all effect modes
void webserver_handle_modes() {
    // * Return a html table with all modes the library provides
    webserver.send(200, "text/plain", modes);
}

// * Handle json input string
void webserver_handle_json() {
    // * Handle json input from http webserver
    String json_input = webserver.arg("plain");

    #ifdef DEBUG_PRINT
    Serial.print(F("HTTP Incoming: "));
    Serial.println(json_input);
    #endif

    bool result = process_json_input((char*) json_input.c_str());

    if (result) {
        send_mqtt_state();
        webserver.send(200, "application/json", F("{\"success\":\"true\",\"message\":\"OK\"}"));
    } else {
        #ifdef DEBUG_PRINT
        Serial.println(F("Error input from HTTP input: parseObject() failed."));
        #endif
        webserver.send(500, "application/json", F("{\"success\":\"false\",\"message\":\"Update Failed\"}"));
    }
}

// * Return the status of the ledstrip as a json string
void webserver_handle_status() {
    // * Update the current state
    update_json_output_buffer();

    // * Send the current json status to the client
    webserver.send(200, "application/json", LEDSTRIP_JSON_OUTPUT);
}

// * Return ON or OFF depending on the state of the ledstrip
void webserver_handle_on_or_off_state() {
    const char* state = get_on_or_off_state();
    webserver.send(200, "text/plain", state);
}

// * Handle input commands from the web interface
void webserver_handle_set() {
    for (uint8_t argument=0; argument < webserver.args(); argument++){

        // * Process Color
        if (webserver.argName(argument) == "c") {
            uint32_t color = (uint32_t) strtol(&webserver.arg(argument)[0], NULL, 16);
            if(color >= 0x000000 && color <= 0xFFFFFF) {
                ledstrip.setColor(color);
            }
        }

        // * Process FX Mode
        if (webserver.argName(argument) == "m") {
            uint8_t led_mode = (uint8_t) strtol(&webserver.arg(argument)[0], NULL, 10);
            ledstrip.setMode(led_mode % ledstrip.getModeCount());
        }

        // * Process Brightness
        if (webserver.argName(argument) == "b") {
            if(webserver.arg(argument)[0] == '-') {
                ledstrip.decreaseBrightness(BRIGHTNESS_STEP);
            } else {
                ledstrip.increaseBrightness(BRIGHTNESS_STEP);
            }
        }

        // * Process Speed
        if (webserver.argName(argument) == "s") {
            if(webserver.arg(argument)[0] == '-') {
                ledstrip.decreaseSpeed(SPEED_STEP);
            } else {
                ledstrip.increaseSpeed(SPEED_STEP);
            }
        }
    }

    send_mqtt_state();
    webserver.send(200, "text/plain", F("OK"));
}

// * Return a 404 when the page does not exist
void webserver_handle_not_found() {
    // * Handle 404 not found error
    webserver.send(404, "text/plain", F("File Not Found"));
}

// * Get all effect modes from ws2801 library and create a html object that's included in the frontpage
void setup_led_html_modes() {
    for(uint8_t i=0; i < ledstrip.getModeCount(); i++) {
        modes += "<li><a href='#' class='m' id='";
        modes += i;
        modes += "'>";
        modes += ledstrip.getModeName(i);
        modes += "</a></li>";
    }
}

// * Setup the webserver
void setup_webserver() {

    #ifdef DEBUG_PRINT
    Serial.println(F("HTTP Webserver setup"));
    #endif

    webserver.on("/",          webserver_handle_index_html);
    webserver.on("/main.js",   webserver_handle_main_js);
    webserver.on("/modes",     webserver_handle_modes);
    webserver.on("/set",       webserver_handle_set);
    webserver.on("/on-or-off", webserver_handle_on_or_off_state);
    webserver.on("/status",    webserver_handle_status);
    webserver.on("/json",      webserver_handle_json);

    webserver.onNotFound(webserver_handle_not_found);
    webserver.begin();

    #ifdef DEBUG_PRINT
    Serial.println(F("HTTP Webserver Started"));
    #endif
}


// **********************************
// * MQTT                           *
// **********************************

// * Send the current state to the MQTT broker
void send_mqtt_state() {
    update_json_output_buffer();

    #ifdef DEBUG_PRINT
    Serial.print(F("MQTT Outgoing: "));
    Serial.println(LEDSTRIP_JSON_OUTPUT);
    #endif

    bool result = mqtt_client.publish(MQTT_OUT_TOPIC, LEDSTRIP_JSON_OUTPUT, true);

    if (!result) {
        #ifdef DEBUG_PRINT
        Serial.println(F("MQTT publish failed "));
        #endif
    }
}

// * Callback for incoming MQTT messages
void mqtt_callback(char* topic, byte* payload_in, unsigned int length) {

    char* payload = (char *) malloc(length + 1);
    memcpy(payload, payload_in, length);
    payload[length] = 0;

    #ifdef DEBUG_PRINT
    Serial.print(F("MQTT Incoming: "));
    Serial.println(payload);
    #endif

    bool result = process_json_input(payload);
    free(payload);

    // * If processing ran successfully, send state to mqtt out topic
    if (result) {
        send_mqtt_state();
    } else {
        #ifdef DEBUG_PRINT
        Serial.println(F("Error input from MQTT broker: parseObject() failed."));
        #endif
    }
}

// * Reconnect to MQTT server and subscribe to in and out topics
bool mqtt_reconnect() {
    // * Loop until we're reconnected
    int MQTT_RECONNECT_RETRIES = 0;

    while (!mqtt_client.connected() && MQTT_RECONNECT_RETRIES < MQTT_MAX_RECONNECT_TRIES) {
        MQTT_RECONNECT_RETRIES++;

        #ifdef DEBUG_PRINT
        Serial.printf("MQTT connection attempt %d / %d ...\n", MQTT_RECONNECT_RETRIES, MQTT_MAX_RECONNECT_TRIES);
        #endif

        // * Attempt to connect
        if (mqtt_client.connect(HOSTNAME, MQTT_USER, MQTT_PASS)) {

            #ifdef DEBUG_PRINT
            Serial.println(F("MQTT connected!"));
            #endif

            // * Once connected, publish an announcement...
            char * message = new char[23 + strlen(HOSTNAME) + 1];
            strcpy(message, "ws2801 ledstrip alive: ");
            strcat(message, HOSTNAME);
            mqtt_client.publish("hass/status", message);

            // * Resubscribe to the incoming mqtt topic
            mqtt_client.subscribe(MQTT_IN_TOPIC);

            #ifdef DEBUG_PRINT
            Serial.printf("MQTT topic in: %s\n", MQTT_IN_TOPIC);
            Serial.printf("MQTT topic out: %s\n", MQTT_OUT_TOPIC);
            #endif

        } else {

            #ifdef DEBUG_PRINT
            Serial.print(F("MQTT Connection failed: rc="));
            Serial.println(mqtt_client.state());
            Serial.println(F(" Retrying in 5 seconds"));
            Serial.println("");
            #endif

            // * Wait 5 seconds before retrying
            delay(5000);
        }
    }

    if (MQTT_RECONNECT_RETRIES >= MQTT_MAX_RECONNECT_TRIES) {
        #ifdef DEBUG_PRINT
        Serial.printf("*** MQTT connection failed, giving up after %d tries ...\n", MQTT_RECONNECT_RETRIES);
        #endif

        return false;
    }

    return true;
}

// **********************************
// * EEPROM helpers                 *
// **********************************

String read_eeprom(int offset, int len) {

    String res = "";
    for (int i = 0; i < len; ++i) {
        res += char(EEPROM.read(i + offset));
    }

    #ifdef DEBUG_PRINT
    Serial.print(F("read_eeprom(): "));
    Serial.println(res.c_str());
    #endif

    return res;
}

void write_eeprom(int offset, int len, String value) {

    #ifdef DEBUG_PRINT
    Serial.print(F("write_eeprom(): "));
    Serial.println(value.c_str());
    #endif

    for (int i = 0; i < len; ++i) {
        if ((unsigned)i < value.length()) {
            EEPROM.write(i + offset, value[i]);
        } else {
            EEPROM.write(i + offset, 0);
        }
    }
}

// ******************************************
// * Callback for saving WIFI config        *
// ******************************************

bool shouldSaveConfig = false;

// * Callback notifying us of the need to save config
void save_wifi_config_callback () {

    #ifdef DEBUG_PRINT
    Serial.println(F("Should save config"));
    #endif

    shouldSaveConfig = true;
}

// **********************************
// * Setup OTA                      *
// **********************************

void setup_ota() {

    #ifdef DEBUG_PRINT
    Serial.println(F("Arduino OTA activated."));
    #endif

    // * Port defaults to 8266
    ArduinoOTA.setPort(8266);

    // * Set hostname for OTA
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    #ifdef DEBUG_PRINT
    ArduinoOTA.onStart([]() {
        Serial.println(F("Arduino OTA: Start"));
    });

    ArduinoOTA.onEnd([]() {
        Serial.println(F("Arduino OTA: End (Running reboot)"));
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Arduino OTA Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Arduino OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println(F("Arduino OTA: Auth Failed"));
        else if (error == OTA_BEGIN_ERROR)
            Serial.println(F("Arduino OTA: Begin Failed"));
        else if (error == OTA_CONNECT_ERROR)
            Serial.println(F("Arduino OTA: Connect Failed"));
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println(F("Arduino OTA: Receive Failed"));
        else if (error == OTA_END_ERROR)
            Serial.println(F("Arduino OTA: End Failed"));
    });
    #endif

    ArduinoOTA.begin();

    #ifdef DEBUG_PRINT
    Serial.println(F("Arduino OTA finished"));
    #endif
}


// **********************************
// * Setup MDNS discovery service   *
// **********************************

void setup_mdns() {
    bool mdns_result = MDNS.begin(HOSTNAME);

    #ifdef DEBUG_PRINT
    Serial.println(F("Starting MDNS responder service"));
    #endif

    if (mdns_result) {
        MDNS.addService("http", "tcp", 80);
    }
}


// **********************************
// * Setup Main                     *
// **********************************

void setup(){
    // * Configure Serial and EEPROM
    Serial.begin(BAUD_RATE);
    EEPROM.begin(512);

    // * Set led pin as output
    pinMode(BUILTIN_LED, OUTPUT);

    // * Start ticker with 0.5 because we start in AP mode and try to connect
    ticker.attach(0.6, tick);

    // * Get MQTT Server settings
    String settings_available = read_eeprom(134, 1);

    if (settings_available == "1") {
        read_eeprom(0, 64).toCharArray(MQTT_HOST, 64);   // * 0-63
        read_eeprom(64, 6).toCharArray(MQTT_PORT, 6);    // * 64-69
        read_eeprom(70, 32).toCharArray(MQTT_USER, 32);  // * 70-101
        read_eeprom(102, 32).toCharArray(MQTT_PASS, 32); // * 102-133
    }

    WiFiManagerParameter CUSTOM_MQTT_HOST("host", "MQTT hostname", MQTT_HOST, 64);
    WiFiManagerParameter CUSTOM_MQTT_PORT("port", "MQTT port",     MQTT_PORT, 6);
    WiFiManagerParameter CUSTOM_MQTT_USER("user", "MQTT user",     MQTT_USER, 32);
    WiFiManagerParameter CUSTOM_MQTT_PASS("pass", "MQTT pass",     MQTT_PASS, 32);

    // * WiFiManager local initialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // * Reset settings - uncomment for testing
    // wifiManager.resetSettings();

    // * Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(configModeCallback);

    // * Set timeout
    wifiManager.setConfigPortalTimeout(WIFI_TIMEOUT);

    // * Set save config callback
    wifiManager.setSaveConfigCallback(save_wifi_config_callback);

    // * Add all your parameters here
    wifiManager.addParameter(&CUSTOM_MQTT_HOST);
    wifiManager.addParameter(&CUSTOM_MQTT_PORT);
    wifiManager.addParameter(&CUSTOM_MQTT_USER);
    wifiManager.addParameter(&CUSTOM_MQTT_PASS);

    // * Fetches SSID and pass and tries to connect
    // * Reset when no connection after 10 seconds
    if (!wifiManager.autoConnect()) {

        #ifdef DEBUG_PRINT
        Serial.println(F("Failed to connect to WIFI and hit timeout"));
        #endif

        // * Reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(WIFI_TIMEOUT);
    }

    // * Read updated parameters
    strcpy(MQTT_HOST, CUSTOM_MQTT_HOST.getValue());
    strcpy(MQTT_PORT, CUSTOM_MQTT_PORT.getValue());
    strcpy(MQTT_USER, CUSTOM_MQTT_USER.getValue());
    strcpy(MQTT_PASS, CUSTOM_MQTT_PASS.getValue());

    // * Save the custom parameters to FS
    if (shouldSaveConfig) {
        #ifdef DEBUG_PRINT
        Serial.println(F("Saving WiFiManager config"));
        #endif

        write_eeprom(0, 64, MQTT_HOST);   // * 0-63
        write_eeprom(64, 6, MQTT_PORT);   // * 64-69
        write_eeprom(70, 32, MQTT_USER);  // * 70-101
        write_eeprom(102, 32, MQTT_PASS); // * 102-133
        write_eeprom(134, 1, "1");        // * 134 --> always "1"
        EEPROM.commit();
    }

    // * If you get here you have connected to the WiFi
    #ifdef DEBUG_PRINT
    Serial.println(F("Connected to WIFI..."));
    #endif

    // * Keep LED on
    ticker.detach();
    digitalWrite(BUILTIN_LED, LOW);

    // * Configure OTA
    setup_ota();

    // * Configure LED Controller
    setup_led_controller();

    // * Fill Controller Modes HTML
    setup_led_html_modes();

    // * Setup Webserver
    setup_webserver();

    // * Startup MDNS Service
    setup_mdns();

    // * Configure MQTT
    snprintf(MQTT_IN_TOPIC, sizeof MQTT_IN_TOPIC, "%s/in", HOSTNAME);
    snprintf(MQTT_OUT_TOPIC, sizeof MQTT_OUT_TOPIC, "%s/out", HOSTNAME);

    mqtt_client.setServer(MQTT_HOST, atoi(MQTT_PORT));
    mqtt_client.setCallback(mqtt_callback);

    #ifdef DEBUG_PRINT
    Serial.printf("MQTT active: %s:%s\n", MQTT_HOST, MQTT_PORT);
    #endif
}


// **********************************
// * Loop                           *
// **********************************

void loop() {
    ledstrip.service();
    webserver.handleClient();
    ArduinoOTA.handle();

    if (!mqtt_client.connected()) {
        long now = millis();

        if (now - LAST_RECONNECT_ATTEMPT > 5000) {
            LAST_RECONNECT_ATTEMPT = now;

            if (mqtt_reconnect()) {
                LAST_RECONNECT_ATTEMPT = 0;
            }
        }
    } else {
        mqtt_client.loop();
    }
}
