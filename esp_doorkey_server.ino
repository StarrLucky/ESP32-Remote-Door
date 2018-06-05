#include <ESP8266WiFi.h>
#include <time.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

const char* ssid = "Lan_indoor";
const char* password = "luckys322";
#define ON   0
#define OFF  1   // JEBANY RELAY MODULE ACTIVE LOW
int pinOpen = 14;  // D7
int pinClose = 12; // D8

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
WiFiUDP Udp;
// NTP-серверы:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";
const int timeZone = 3;     // Рашка GMT+3
unsigned int localPort = 8888;  // локальный порт для прослушивания UDP-пакетов
time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

void setup() {
  Serial.begin(9600);
  delay(10);

  // prepare Dpins
 pinMode(pinOpen, OUTPUT);
 pinMode(pinClose, OUTPUT);
 digitalWrite(pinOpen, OFF);
 digitalWrite(pinClose, OFF);
  
  // Connect to WiFi network
  Serial.println();
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
  // Start the server
  server.begin();
  Serial.println("Server started");
  // Print the IP address
  Serial.print("IP number assigned by DHCP is ");  //  "IP, присвоенный DHCP: "
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");  //  "Начинаем UDP"
  Udp.begin(localPort);
  Serial.print("Local port: ");  //  "Локальный порт: "
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");  //  "ждем синхронизации"
 // setSyncProvider(getNtpTime);
//  setSyncInterval(300);
  
}

void loop() {


 
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // Match the request
  int val;
  if (req.indexOf("/open") != -1)
    val = 1;
  else if (req.indexOf("/close") != -1)
    val = 0;
  else {
    String s = "HTTP/1.1 204 No Content\r\n";
    client.print(s);
    Serial.println("invalid request");
    client.stop();
    return;
  }

  // Set GPIO2 according to the request
if ((timeStatus() != timeNotSet)) 
{
if ((hour()>8)&&(hour()<18)) 
{  
  switch (val) {
    case 1: // открыть 
    {
    String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n Openning..";
    client.print(s);
    digitalWrite(pinClose, OFF);
    digitalWrite(pinOpen, ON);
    delay(1000);
    digitalWrite(pinOpen, OFF);
   // String s = "Door is now UnSherlocked!";


    break;
    }
    case 0:  //закрыть
    {
    String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n Closing..watch out, watch out!";

    client.print(s);
    digitalWrite(pinOpen, OFF);
    digitalWrite(pinClose, ON);
    delay(1000);
    digitalWrite(pinClose, OFF);
    //String s = "Door is now Sherlocked!";

    break;
  }
    default:
    {
      Serial.println("Обнаружена попытка взлома. Я вызываю полицию;) POSHOL NAHUJ PIDAR");
    }
    
}
}
else if  ((hour()>18)||(hour()<8)) {
   String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n В данный час (";
s += hour();
s+= ") часа(ов) управление дверями заблокировано.";
    client.print(s);
  

}

}
else if ((timeStatus() == timeNotSet)) 
{
   String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n На сервере не установлено время. Управление дверями заблокировано";
    client.print(s);
}

  
  client.flush();

  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

/*-------- Код для NTP ----------*/
 
const int NTP_PACKET_SIZE = 48;  //  NTP-время – в первых 48 байтах сообщения
byte packetBuffer[NTP_PACKET_SIZE];  //  буфер для хранения входящих и исходящих пакетов 
 
time_t getNtpTime()
{
  IPAddress ntpServerIP; // IP-адрес NTP-сервера
 
  while (Udp.parsePacket() > 0) ; // отбраковываем все пакеты, полученные ранее 
  Serial.println("Transmit NTP Request");  //  "Передача NTP-запроса" 
  // подключаемся к случайному серверу из списка:
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");  //  "Получение NTP-ответа"
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // считываем пакет в буфер 
      unsigned long secsSince1900;
      // конвертируем 4 байта (начиная с позиции 40) в длинное целое число: 
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");  //  "Нет NTP-ответа :(" 
  return 0; // если время получить не удалось, возвращаем «0» 
}
 
// отправляем NTP-запрос серверу времени по указанному адресу: 
void sendNTPpacket(IPAddress &address)
{
  // задаем все байты в буфере на «0»: 
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // инициализируем значения для создания NTP-запроса
  // (подробнее о пакетах смотрите по ссылке выше) 
  packetBuffer[0] = 0b11100011;   // LI (от «leap indicator», т.е. «индикатор перехода»), версия, режим работы 
  packetBuffer[1] = 0;     // слой (или тип часов) 
  packetBuffer[2] = 6;     // интервал запросов 
  packetBuffer[3] = 0xEC;  // точность 
  // 8 байтов с нулями, обозначающие базовую задержку и базовую дисперсию: 
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // После заполнения всех указанных полей
  // вы сможете отправлять пакет с запросом о временной метке:      
  Udp.beginPacket(address, 123); // NTP-запросы к порту 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
