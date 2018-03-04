// **********************************
// * Ledstrip Settings              *
// **********************************

// * Data pins and led count
#define LED_DATA_PIN 3
#define LED_CLOCK_PIN 2

#define BAUD_RATE 115200

// Ledstrip 1
//#define LED_COUNT 45
//#define LED_PIXEL_ORDER WS2801_RBG
//#define HOSTNAME "flipstrip1.home"

// Ledstrip 2
//#define LED_COUNT 25
//#define LED_PIXEL_ORDER WS2801_RBG
//#define HOSTNAME "flipstrip2.home"

// Ledstrip 3
//#define LED_COUNT 19
//#define LED_PIXEL_ORDER WS2801_RBG
//#define HOSTNAME "flipstrip3.home"

// Ledstrip 4
#define LED_COUNT 4
#define LED_PIXEL_ORDER WS2801_BGR
#define HOSTNAME "flipstrip4.home"

// * Turn on debug print to serial device
#define DEBUG_PRINT

// * The password used for uploading
#define OTA_PASSWORD "admin"

// * Webserver http port to listen on
#define HTTP_PORT 80

// * Wifi timeout in milliseconds
#define WIFI_TIMEOUT 30000

// * Startup defaults for the led strip
#define DEFAULT_BASE_COLOR 0xFF5900
#define DEFAULT_BASE_BRIGHTNESS 255
#define DEFAULT_BASE_SPEED 200
#define DEFAULT_BASE_MODE FX_MODE_STATIC

// * Increase / decrease brightness and speed with this amount per click
#define BRIGHTNESS_STEP 15
#define SPEED_STEP 10

// * Define on/off state for ledstrip
#define STATE_OFF "OFF"
#define STATE_ON "ON"

// * MQTT network settings
#define MQTT_MAX_PACKET_SIZE 512
#define MQTT_MAX_RECONNECT_TRIES 10

// * To be filled with EEPROM data
char MQTT_HOST[64] = "";
char MQTT_PORT[6]  = "";
char MQTT_USER[32] = "";
char MQTT_PASS[32] = "";

// * MQTT in and out topic based on hostname
char MQTT_IN_TOPIC[strlen(HOSTNAME) + 4];      // * Topic in will be: <HOSTNAME>/in
char MQTT_OUT_TOPIC[strlen(HOSTNAME) + 5];     // * Topic out will be: <HOSTNAME>/out

// * MQTT Last reconnection counter
long LAST_RECONNECT_ATTEMPT = 0;

// * Buffer size for json input stream
const int BUFFER_SIZE = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 215;

// * The result of json parsing in as a char array
char LEDSTRIP_JSON_OUTPUT[100];

// * Will be filled with html for modes listing
String modes = "";
