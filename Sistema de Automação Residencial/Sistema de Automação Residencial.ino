//Bibliotecas usadas
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

//pinos usados
#define Lampada 2
#define Tomada 15
#define movime 21
#define Light 32

//token do bot do telegram
#define BOT_TOKEN "5756020379:AAFavtnJHQSqtln3eVzol0l4IL0swHcnYzs"
#define WIFI_SSID "brisa-2553990"
#define PASSWORD "toucsi8d"


//variaveis para comandos no telegram
const String LIGHT_ON = "Ligar a luz";
const String LIGHT_OFF = "Desligar a luz";
const String TOMADA_ON = "Ligar a tomada";
const String TOMADA_OFF = "Desligar a tomada";
const String ILUMINACAO = "Sensor de iluminação";
const String ATIDESILUMI = "Ativar/Desativar o sensor de iluminação";
const String ATIDESPRESE = "Ativar/Desativar o sensor de presença";
const String RSTPRIORI = "Resetar Prioridade";
const String STATS = "Status";
const String START = "/start";

//intervalos para calculo de tempo
const unsigned long intervalo = 1000;
const unsigned long intervalosensor = 50;
const unsigned long intervaloligado = 15000;

//variaveis para controle
int statusdalampada = LOW;
int statusdatomada = LOW;
int sensorilumi = 1;
int sensorprese = 1;
int prioridadesobreilum = 1;
int prioridadesobrepres = 0;
int val;

//config wifi e telegram
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
#define SENDER_ID_COUNT 1
String validSenderIds[SENDER_ID_COUNT] = {"627434486"};

//variaveis para controlar ultima interação
unsigned long lastCheckTime;
unsigned long ultimovi;
unsigned long ultilumi;
unsigned long ultimacion;

//setup
void setup()
{
  //iniciar serial
  Serial.begin(115200);
  
  //conectar wifi
  setupWiFi();

  //config da pinagens
  pinMode(Lampada, OUTPUT);
  digitalWrite(Lampada, statusdalampada);
  pinMode(Tomada, OUTPUT);
  digitalWrite(Tomada, statusdatomada);
  pinMode(movime, INPUT);
}

void setupWiFi()
{
  Serial.print("Conectando-se ao SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Conectado!");
}

void loop()
{
  //sensor ldr
  if (millis() - ultilumi > intervalosensor and sensorilumi == 1 and prioridadesobreilum == 0)
  {
    val = analogRead(Light);
    if(val > 3800) {
      statusdalampada = HIGH;
      digitalWrite(Lampada, statusdalampada);
      String messageilu = "A luz foi ligada pelo sensor de iluminação\n";
      bot.sendMessage("627434486", messageilu, "HTML");
    }
    else {
      statusdalampada = LOW;
      digitalWrite(Lampada, statusdalampada);
    }
    Serial.println("Luminosidade: ");
    Serial.println(val);
    ultilumi = millis();
  }
  //sensor movimento pir
  if (millis() - ultimovi > intervalosensor and sensorprese == 1 and prioridadesobrepres == 0)
  {
    bool valormovim = digitalRead(movime);
  
    if(valormovim){
      Serial.println("DETECTADO MOVIMENTO");
      if (millis() - ultimacion > intervaloligado){
        statusdalampada = HIGH;
        statusdatomada = HIGH;
        digitalWrite(Lampada, statusdalampada);
        digitalWrite(Tomada, statusdatomada);
        String messagemovi = "Luz e tomada foram ligados pelo sensor de movimento\n";
        bot.sendMessage("627434486", messagemovi, "HTML");
        ultimacion = millis();
      }
    }
    else {
      Serial.println("NENHUM MOVIMENTO");
      statusdalampada = LOW;
      statusdatomada = LOW;
      digitalWrite(Lampada, statusdalampada);
      digitalWrite(Tomada, statusdatomada);
    }
    ultimovi = millis();
  }
  //checagem de mensagem no telegram
  if (millis() - lastCheckTime > intervalo)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastCheckTime = millis();
  }
}

void handleNewMessages(int numNewMessages)
{
  for (int i=0; i<numNewMessages; i++)
  {
    String chatId = String(bot.messages[i].chat_id);
    String senderId = String(bot.messages[i].from_id);
    Serial.println("senderId: " + senderId);
    boolean validSender = validateSender(senderId);
    if(!validSender)
    {
      bot.sendMessage(chatId, "Desculpe mas você não tem permissão", "HTML");
      continue;
    }
    String text = bot.messages[i].text;
    if (text.equalsIgnoreCase(START))
    {
      handleStart(chatId, bot.messages[i].from_name);
    }
    else if (text.equalsIgnoreCase(LIGHT_ON))
    {
      handleLightOn(chatId);
    }
    else if(text.equalsIgnoreCase(LIGHT_OFF))
    {
     handleLightOff(chatId);
    }
    else if(text.equalsIgnoreCase(TOMADA_ON))
    {
      handleTomadaOn(chatId);
    }
    else if(text.equalsIgnoreCase(TOMADA_OFF))
    {
      handleTomadaOff(chatId);
    }
    else if(text.equalsIgnoreCase(ILUMINACAO))
    {
      handleIluminacao(chatId);
    }
    else if(text.equalsIgnoreCase(ATIDESILUMI))
    {
      handleAtiDesIlumi(chatId);
    }
    else if(text.equalsIgnoreCase(ATIDESPRESE))
    {
      handleAtiDesPrese(chatId);
    }
    else if(text.equalsIgnoreCase(RSTPRIORI))
    {
      handleRstPriori(chatId);
    }
    else if (text.equalsIgnoreCase(STATS))
    {
      handleStatus(chatId);
    }
    else
    {
      handleNotFound(chatId);
    }
  }
}

boolean validateSender(String senderId)
{
  for(int i=0; i<SENDER_ID_COUNT; i++)
  {
    if(senderId == validSenderIds[i])
    {
      return true;
    }
  }
  return false;
}

void handleStart(String chatId, String fromName)
{
  String message = "<b>Olá " + fromName + ".</b>\n";
  message += getCommands();
  bot.sendMessage(chatId, message, "HTML");
}

String getCommands()
{
  String message = "Os comandos disponíveis são:\n\n";
  message += "<b>" + LIGHT_ON + "</b>: Para ligar a luz\n";
  message += "<b>" + LIGHT_OFF + "</b>: Para desligar a luz\n";
  message += "<b>" + TOMADA_ON + "</b>: Para ligar a tomada\n";
  message += "<b>" + TOMADA_OFF + "</b>: Para desligar a tomada\n";
  message += "<b>" + ILUMINACAO + "</b>: Para verificar o sensor de iluminação\n";
  message += "<b>" + ATIDESILUMI + "</b>: Para ativar/desativar o sensor de iluminação\n";
  message += "<b>" + ATIDESPRESE + "</b>: Para ativar/desativar o sensor de presença\n";
  message += "<b>" + RSTPRIORI + "</b>: Para resetar a prioridade sobre os sensores\n";
  message += "<b>" + STATS + "</b>: Para verificar o estado da luz, da tomada e dos sensores";
  return message;
}

void handleLightOn(String chatId)
{
  prioridadesobreilum = 1;
  prioridadesobrepres = 1;
  statusdalampada = HIGH;
  digitalWrite(Lampada, statusdalampada);
  bot.sendMessage(chatId, "A luz está <b>acesa</b>", "HTML");
}

void handleLightOff(String chatId)
{
  prioridadesobreilum = 1;
  prioridadesobrepres = 1;
  statusdalampada = LOW;
  digitalWrite(Lampada, statusdalampada);  
  bot.sendMessage(chatId, "A luz está <b>apagada</b>", "HTML");
}

void handleTomadaOn(String chatId)
{
  prioridadesobrepres = 1;
  statusdatomada = HIGH;
  digitalWrite(Tomada, statusdatomada);  
  bot.sendMessage(chatId, "A tomada está <b>ligada</b>", "HTML");
}

void handleTomadaOff(String chatId)
{
  prioridadesobrepres = 1;
  statusdatomada = LOW;
  digitalWrite(Tomada, statusdatomada);  
  bot.sendMessage(chatId, "A tomada está <b>desligada</b>", "HTML");
}

void handleIluminacao(String chatId)
{
  if(sensorilumi == 1){
    bot.sendMessage(chatId, getIluminacaoMessage(), "");
  }
  else{
  }bot.sendMessage(chatId, "O sensor está desativado", "HTML");
}

String getIluminacaoMessage()
{
//  float temperature, humidity;
//  int status = dht.read2(DHT_PIN, &temperature, &humidity, NULL);
//
//  if (status == SimpleDHTErrSuccess)
//  {
//    String message = "";
//    message += "A temperatura é de " + String(temperature)+ " °C e ";
//    message += "a umidade é de " + String(humidity) + "%";
//    return message;
//  }
  return "Carregando luminosidade...";
}

void handleAtiDesIlumi(String chatId)
{
  if(sensorilumi == 1){
    bot.sendMessage(chatId, "O sensor de iluminação foi <b>desativado</b>", "HTML");
  }
  else{
    bot.sendMessage(chatId, "O sensor de iluminação foi <b>ativado</b>", "HTML");
  }
  sensorilumi = ~sensorilumi;
}

void handleAtiDesPrese(String chatId)
{
  if(sensorprese == 1){
    bot.sendMessage(chatId, "O sensor de presença foi <b>desativado</b>", "HTML");
  }
  else{
    bot.sendMessage(chatId, "O sensor de presença foi <b>ativado</b>", "HTML");
  }
  sensorprese = ~sensorprese;
}

void handleRstPriori(String chatId){
  if(prioridadesobreilum == 1 or prioridadesobrepres == 1){
    bot.sendMessage(chatId, "A prioridade foi <b>resetada</b>", "HTML");
    prioridadesobreilum = 0;
    prioridadesobrepres = 0;
  }
  else{
    bot.sendMessage(chatId, "Os sensores já possuem a prioridade", "HTML");
  }
}

void handleStatus(String chatId)
{
  String message = "";
  if(statusdalampada == HIGH)
  {
    Serial.println("LAMPADA LIGADA"+statusdalampada);
    message += "A luz está acesa\n";
  }
  else
  {
    Serial.println("LAMPADA DESLIGADA"+statusdalampada);
    message += "A luz está apagada\n";
  }

  if(statusdatomada == HIGH)
  {
    Serial.println("TOMADA LIGADA"+statusdatomada);
    message += "A tomada está ligada\n";
  }
  else
  {
    Serial.println("TOMADA DESLIGADA"+statusdatomada);
    message += "A tomada está desligada\n";
  }
  
  if(sensorilumi == 1)
  {
    Serial.println("SENSOR ILUMINAÇÃO LIGADO"+sensorilumi);
    message += "O sensor de iluminação está ligado\n";
  }
  else
  {
    Serial.println("SENSOR ILUMINAÇÃO DESLIGADO"+sensorilumi);
    message += "O sensor de iluminação está desligado\n";
  }

  if(sensorprese == 1)
  {
    Serial.println("SENSOR PRESENÇA LIGADO"+sensorprese);
    message += "O sensor de presença está ligado\n";
  }
  else
  {
    Serial.println("SENSOR PRESENÇA DESLIGADO"+sensorprese);
    message += "O sensor de presença está desligado\n";
  }

  if(prioridadesobreilum == 1)
  {
    Serial.println("SENSOR ILUMINAÇÃO SEM"+prioridadesobreilum);
    message += "O sensor de iluminação está sem prioridade\n";
  }
  else
  {
    Serial.println("SENSOR ILUMINAÇÃO COM"+prioridadesobreilum);
    message += "O sensor de iluminação está com prioridade\n";
  }

  if(prioridadesobrepres == 1)
  {
    Serial.println("SENSOR PRESENÇA SEM"+prioridadesobrepres);
    message += "O sensor de presença está sem prioridade\n";
  }
  else
  {
    Serial.println("SENSOR PRESENÇA COM"+prioridadesobrepres);
    message += "O sensor de presença está com prioridade\n";
  }
  message += getIluminacaoMessage();
  bot.sendMessage(chatId, message, "");
}

void handleNotFound(String chatId)
{
  String message = "Comando não encontrado\n";
  message += getCommands();
  bot.sendMessage(chatId, message, "HTML");
}
