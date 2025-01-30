#include <AccelStepper.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h> // Для работы с MQTT

const int relay = 5;

#define MQTT_VERSION MQTT_VERSION_3_1_1 // Устанавливаем версию MQTT

String clientId = "ESP8266-fam_rel"; // Уникальный ID клиента для MQTT
#define MQTT_ID "/ESP8266-fam_rel/"
#define MQTT_RELAY "/ESP8266-fam_rel/fam/" // Топик для устройства
#define PUB_RELAY "/ESP8266-fam_rel/fam_relay/" // Топик для отправки состояния двигателя
#define MSG_BUFFER_SIZE 20

const char *Topic = MQTT_RELAY; // Топик для устройства

bool flag; // Переменная для хранения значений для двигателя
char m_msg_buffer[MSG_BUFFER_SIZE]; // Буфер для получения сообщений

// Данные Wi-Fi
const char* ssid = "ssid";
const char* password = "password";
const char *mqtt_server = "m6.wqtt.ru"; // Адрес MQTT-сервера
const char *mqtt_user = "user"; // Логин для MQTT (если требуется)
const char *mqtt_pass = "password"; // Пароль для MQTT (если требуется)
WiFiClient espClient; // Объект для подключения к Wi-Fi
PubSubClient client(espClient); // Объект для работы с MQTT

const char *p_payload; // Переменная для хранения полученных данных
float got_float; // Переменная для преобразования данных в число
int32_t got_int;
int i; // Переменная для циклов

// Пин для LED
const int ledPin = 2; // Встроенный светодиод на ESP8266 (может быть GPIO2)

bool connectWiFi(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA); // Режим клиента Wi-Fi
  WiFi.begin(ssid, password);

  int connectionTimeout = 60; // Тайм-аут подключения (в секундах)
  pinMode(ledPin, OUTPUT);

  Serial.println("Waiting for Wi-Fi connection...");
  while (WiFi.status() != WL_CONNECTED && connectionTimeout > 0) {
    digitalWrite(ledPin, !digitalRead(ledPin)); // Переключение LED
    Serial.println(WiFi.status()); // Статус подключения
    delay(1000); // Ожидание 1 секунду
    connectionTimeout--;
  }

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(ledPin, LOW); // Отключение мигания LED
    Serial.println("Connection successful!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP()); // Вывод IP-адреса
    return true;
  } else {
    digitalWrite(ledPin, HIGH); // Светодиод выключен при неудаче
    Serial.println("Failed to connect to Wi-Fi");
    return false;
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  // Функция обработки полученных сообщений по MQTT
  for (i = 0; i < length; i++) {
    m_msg_buffer[i] = payload[i]; // Копируем данные из payload в буфер
  }
  m_msg_buffer[i] = '\0'; // Завершаем строку
  p_payload = m_msg_buffer; // Указываем на начало строки
  got_float = atof(p_payload); // Преобразуем строку в число с плавающей точкой

  got_int = (int)got_float;
  if (got_int == 0) {
    digitalWrite(relay, HIGH); // Выключаем реле
  } else if (got_int == 1) {
    digitalWrite(relay, LOW); // Включаем реле
  }
  snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", got_int); // Формируем сообщение
  client.publish(PUB_RELAY, m_msg_buffer, true); // Отправляем сообщение в топик
}

void reconnect() {
  // Функция переподключения к MQTT
  while (!client.connected()) {
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) { // Если подключились
      client.subscribe(MQTT_RELAY); // Подписываемся на топик устройства
    } else {
      delay(6000); // Если не удалось подключиться, повторяем попытку через 6 секунд
    }
  }
}

void setup() {
  // initialize the serial port
  Serial.begin(115200);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH); // Выключаем реле
  if (connectWiFi(ssid, password)) {
    client.setServer(mqtt_server, 17888); // Устанавливаем сервер для MQTT
    client.setCallback(callback); // Устанавливаем функцию обратного вызова для MQTT
  }
}

void loop() {
  // Основной цикл программы
  if (!client.connected()) { // Если MQTT не подключен
    reconnect(); // Подключаемся
    delay(1000); // Задержка 1 секунда
  }
  client.loop(); // Обработка входящих сообщений MQTT
}
