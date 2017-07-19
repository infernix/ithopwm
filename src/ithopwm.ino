/*
 Drive an ITHO PWM controller board with an ESP8266

 PWM code from https://github.com/StefanBruens/ESP8266_new_pwm

 Note: specs are 5V-10V, 3.5kHz-4.5kHz; 5V is not enough so drive it at 7.5 or 9v.
 We're using 4kHz PWM in this example. We dim the led based on the duty cycle as well.

 Code was written for a NodeMCU Amica v2 board, may need adaptation for others.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Arduino.h"
#include <Syslog.h>
#include "WiFiUdp.h"

extern "C" {
#include "pwm.h"
}

// Update these with values suitable for your network.
// Syslog server connection info
#define SYSLOG_SERVER "192.168.0.1"
#define SYSLOG_PORT 514
// This device info
#define DEVICE_HOSTNAME "ithopwm"
#define APP_NAME "controller"
// Wifi
const char* ssid = "YOUR_SSID_HERE";
const char* password = "YOUR_WIFI_PASS_HERE";
// MQTT
const char* mqtt_server = "YOUR_MQTT_SERVER_IP_HERE";
const char* mqtt_user = "YOUR_MQTT_USERNAME_HERE";
const char* mqtt_password = "YOUR_MQTT_PASSWORD_HERE";
const char* mqtt_client = "ithopwm";
const char* mqtt_topic = "ithopwm/duty";


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udpClient;

Syslog syslog(udpClient);


// 4khz in 200ns periods = (1000000000 / 4000) / 200 = 250000/200 = 1250
uint32 pwm_period_init = 1250;
uint32 pwm_duty_init[2] = {625,625}; // We default to 50%
uint32 io_info[2][3] = {
    {PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12, 12},
    {PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2, 2},
};

void setup() {
  digitalWrite(D0, HIGH);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  setup_wifi();

  syslog.server(SYSLOG_SERVER, SYSLOG_PORT);
  syslog.deviceHostname(DEVICE_HOSTNAME);
  syslog.appName(APP_NAME);
  syslog.defaultPriority(LOG_DEBUG);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(12, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as a PWM
  pwm_init(pwm_period_init, pwm_duty_init, 2, io_info);
  pwm_start();
}

void setup_wifi() {

  delay(10);
  // First shut down AP
  WiFi.enableAP(false);
  WiFi.softAPdisconnect(true);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  syslog.log(LOG_DEBUG, "Connected with wifi");
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Zero the payload before reading it!
  payload[length] = '\0';
  String dutystring = String((char*)payload);
  int dutypercent = dutystring.toInt();
  // 100% is the max, anything else we just drop
  if (dutypercent > 100) {
    dutypercent = 100;
  }
  float ithoduty = (pwm_period_init/100.0)*dutypercent;
  float ledduty = pwm_period_init-((pwm_period_init/100.0)*dutypercent);
  Serial.print("Set PWM duty % to ");
  Serial.print(dutypercent);
  Serial.print(" (ITHO=");
  Serial.print((int) ithoduty);
  Serial.print("/");
  Serial.print(pwm_period_init);
  Serial.print(", LED=");
  Serial.print((int) ledduty);
  Serial.print("/");
  Serial.print(pwm_period_init);
  Serial.print(")");
  Serial.println();
  syslog.logf(LOG_DEBUG, "Set PWM duty to %d% (ITHO=%d/%d)", dutypercent, (int) ithoduty, pwm_period_init);
  pwm_set_duty((int) ithoduty, 0);
  pwm_set_duty((int) ledduty, 1);
  pwm_start();
  Serial.println();

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    syslog.log("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      syslog.log("connected to MQTT");
      if (client.subscribe(mqtt_topic)) {
        Serial.println("subscribed to topic");
        syslog.logf("subscribed to topic %s", mqtt_topic);
      } else {
        Serial.println("failed to subscribe, try again in 5 seconds");
        syslog.log("failed to subscribe to topic, retrying in 5 seconds");
        delay(5000);
      }
    } else {
      syslog.logf("MQTT connection failed, rc=%d - retrying in 5 seconds", client.state());
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  digitalWrite(D0, HIGH);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  digitalWrite(D0, LOW);

}
