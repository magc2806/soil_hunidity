#include <stdio.h> /* Prototipos de intrada/salida */
#include "WiFi.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30        /* Time ESP32 will go to sleep (in seconds) */
RTC_DATA_ATTR int bootCount = 0; //conteo de veces que despierta

#define TINY_GSM_MODEM_SIM800 // MODULO GSM UTILIZADO (SIM800L)
#define TINY_GSM_RX_BUFFER   1024 //TAMAÑO DEL BUFFER
#include <TinyGsmClient.h> // BIBLIOTECA DE COMANDOS TinyGSM

//---------GSM-----------
HardwareSerial SerialGSM(2); //Uso de modulo UART2 para conexion con el modulo GSM
TinyGsm modemGSM(SerialGSM); // Objeto TinyGsm este es madicamente el modulo GSM
const int BAUD_RATE = 9600; // Velocidad de comunicacion
const int RX_PIN = 16, TX_PIN = 17; //Pines para conectar al modulo GSM

//Credenciales para conectar datos de SIM. Valido para ENTEL 
const char apn[] = "imovil.entelpcs.cl";
const char gprsUser[] = "entelpcs";
const char gprsPass[] = "entelpcs";

//---------Conexión Wifi------------
#define WIFI_SSID "Familia Costanera"
#define PASSWORD "Costanera728"

// ----PARAMETROS DE WEB---
#define TOKEN  "d771ba30a89855600bfc13c1d1d6377d" //Token de la API
#define WIFI_SSID "Familia Costanera"                                   // input your home or public wifi name
#define WIFI_PASSWORD "Costanera728"                                    //password of wifi ssid
String website_URL = "http://evening-headland-87613.herokuapp.com/api/v1/measurements";
int measurement_point_id=1;
const int port = 80;

//------SENSOR------------

const unsigned int humidity_pin = 39;
unsigned int humidity_value=0;

//-----Variables de tiempo-----------
unsigned long current_time = 0; //para ir guardando el tiempo actual

const unsigned long reading_period = 2000; //periodo de lectura de los sensores
unsigned long last_reading_time = 0; // ultimo tiempo de lectura

const unsigned long sending_period = 10000;
unsigned long last_sending_time = 0;



//---LCD---- SDA ES GPIO 21 SCL GPIO22
const int lcdColumns = 16;
const int lcdRows = 2;
//Objeto LCD
LiquidCrystal_I2C lcd(0x27,lcdColumns, lcdRows);

struct Button {
  unsigned int PIN;  
  bool pressed;
};

Button btt1 = {19, false};
Button btt2 = {18, false};

unsigned int opc = 0; //para seleccionar wifi o gsm

unsigned int led = 33;

void IRAM_ATTR isr() {
  
  btt1.pressed = true;
}

void IRAM_ATTR isr2() {
  
  btt2.pressed = true;
}

void ShowSerialData()
{ delay(500);
  while (SerialGSM.available() != 0)
    Serial.write(SerialGSM.read());
}

void setup_wifi(){
  WiFi.mode(WIFI_STA); //Configuro el ESP32 como station mode para que se pueda conectar a alguna red  
  WiFi.begin(WIFI_SSID, PASSWORD);
  delay(100);
  /*WiFi.status() retorna un entero dependiendo del estado en que se encuentre el wifi y lo compara con el enum WL_CONNECTED.
 * Si esta conectado debería retornar 3
 */
  while (WiFi.status() != WL_CONNECTED ){ // mientras no este coenctado envío mensaje de que no se puede conectar a la red
       Serial.println("No se puede conectar a la red");
       delay(500); 
       digitalWrite(led, LOW);
       delay(1000);
       digitalWrite(led, HIGH);
       delay(1000);     
     }  
  Serial.println("Wifi connected");
  digitalWrite(led, HIGH); //enciendo el LED una vez que el wifi está conectado 
  
  }


void setup_gsm(){
  Serial.println("Initializing GSM module...");
  SerialGSM.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN, false);
  delay(3000);
  Serial.println(modemGSM.getModemInfo());
  delay(100);
  if (!modemGSM.init()) {
  // if (!modem.restart()) {
    Serial.println(F(" Restarting"));    
    delay(100);
    return;
  }

  if (!modemGSM.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println("Connection to APN failed");
    }
    else{
      Serial.print("Connected to");
      Serial.println(apn);
    } 

     if (modemGSM.isNetworkConnected()){

      Serial.print("Connected to network");
      digitalWrite(led, HIGH); //enciendo el LED una vez que el wifi está conectado    
      
      }
      else {
        Serial.print("Network connection failed");
        
        }
  }

void show_to_lcd(int opc){
      lcd.clear();
      char data[10];
     if(opc == 1)
     {
        lcd.setCursor(0,0);
        lcd.print("[1]Wifi ");
        lcd.setCursor(0,1);
        lcd.print("[2]Movil ");
        delay(100);      
     }

      else if (opc ==2)
      {
        //lcd.clear();
      //me paro en la primera fila      
        lcd.setCursor(0,0);     
        sprintf(data, "Hum: %d", humidity_value);
        lcd.print(data);
        
      }                
  }

  

void sending_data_wifi(){

  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
 
    StaticJsonBuffer<400> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[400];

    jsonObj["measurement_point_id"] = String(measurement_point_id);
    jsonObj["token"] = String(TOKEN);
    jsonObj["moisture"] = String(humidity_value);
    jsonObj["temperature"] = "250";
    jsonObj["date"] = "2020-02-20 19:00:00";
    jsonObj.printTo(Serial);
    jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
       
    HTTPClient http;    //Declare object of class HTTPClient
 
    http.begin(website_URL);      //Specify request destination
    http.addHeader("Content-Type", "application/json");  //Specify content-type header
 
    int httpCode = http.POST(JSONmessageBuffer);   //Send the request
    String payload = http.getString();                                        //Get the response payload

    Serial.println("End of Json: ");
    Serial.println(httpCode);   //Print HTTP return code
    Serial.println("End of Json 2: ");
    Serial.println(payload);    //Print request response payload
 
    http.end();  //Close connection
        
   }
   else {
 
    Serial.println("Error in WiFi connection");
    //Serial.println("Error in Data connection");
 
   }  
}

  void sending_data_gsm(){
    Serial.print("Connecting to APN ");
    Serial.println(apn);

  if (modemGSM.gprsConnect(apn, gprsUser, gprsPass)) 
  { //Check if APN is connected

    Serial.print("Connected to APN ");
    Serial.println(apn);

    if (modemGSM.isNetworkConnected())
    { //check if network is connected
          Serial.println("Connected to network");
          Serial.print("Connecting to ");
          Serial.print(website_URL);
          Serial.println("Sending AT Command");
          Serial.println("Sending http init");
          SerialGSM.println("AT+HTTPINIT");          
          ShowSerialData();
          delay(1000);
          Serial.println("Sending CID");
          delay(1000);
          SerialGSM.println("AT+HTTPPARA=\"CID\",1");
          ShowSerialData();
          delay(1000);
          SerialGSM.println("AT+HTTPPARA=\"URL\",\"http://evening-headland-87613.herokuapp.com/api/v1/measurements\"");
          ShowSerialData();           
          delay(1000);
          SerialGSM.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
          ShowSerialData();
          delay(1000);
                   
          StaticJsonBuffer<400> jsonBuffer;
          JsonObject& jsonObj = jsonBuffer.createObject();
          char JSONmessageBuffer[400];

          jsonObj["measurement_point_id"] = String(measurement_point_id);
          jsonObj["token"] = String(TOKEN);
          jsonObj["moisture"] = String(humidity_value);
          jsonObj["temperature"] = "250";
          jsonObj["date"] = "2020-02-20 19:00:00";
          jsonObj.printTo(Serial);
          String sendToServer;
          jsonObj.prettyPrintTo(sendToServer);
          SerialGSM.println("AT+HTTPDATA=" + String(sendToServer.length()) + ",100000"); //Aqui le digo el tamaño de lo que se enviara
          ShowSerialData();
          Serial.println("####################");
          //Serial.println(sendToServer);
          
          SerialGSM.println(sendToServer); //Aqui colo el json para ser enviado
          ShowSerialData();
          delay(2000);
          
          SerialGSM.println("AT+HTTPACTION=1"); //Aqui envio el Json
          delay(2000);
          ShowSerialData();

          SerialGSM.println("AT+HTTPREAD"); //Aqui leo si fue enviado con exito. Si regresa 200 fue exitoso
          delay(2000);
          ShowSerialData();

           SerialGSM.println("AT+HTTPTERM"); //Aqui termino la comunidacion HTTP
           delay(1000);
           ShowSerialData();        
    }
    else
    {

      Serial.println("Network Connection failed");
    }
  }

    else 
    {
      Serial.println("Error connecting to APN");
     }

    
    }

 void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

 wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
void setup() {
  // put your setup code here, to run once:  
  pinMode(humidity_pin, INPUT);
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  pinMode(btt1.PIN, INPUT_PULLUP);
  pinMode(btt2.PIN, INPUT_PULLUP);
  attachInterrupt(btt1.PIN, isr, FALLING);
  attachInterrupt(btt2.PIN, isr2, FALLING);
  delay(10);
  Serial.begin(BAUD_RATE);     
  
  lcd.init();                                                     //INICIO LCD
  delay(10);
  
  lcd.backlight();                                                //Enciendo la luz trasera del LCD
  lcd.setCursor(0,0);
  lcd.print("Iniciando...");
  delay(100);
  
  //setup_gsm(); 
  //WiFi.disconnect(); 
  //Serial.println("Wifi disconnected");
  //++bootCount;
//  Serial.println("Boot number: " + String(bootCount));
  delay(1000);
  //print_wakeup_reason();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");
}

void loop() {
  // put your main code here, to run repeatedly:
  //lcd.print("loop");
   

  while(!Serial){
    //Me quedo aquí mientras no haya comunicación serial 
         
    }

    //Me quedo aqui hasta que seleccione un metodo de envio
    while(opc == 0)
    {
        show_to_lcd(1);
        if(btt1.pressed == true) 
        {
          opc = 1; //1 para wifi
          btt1.pressed = false;
          btt2.pressed = false;
          lcd.clear();
          lcd.print("Configurando wifi");
          delay(10);
          Serial.println("Confiruando wifi");
          setup_wifi();
          delay(10);
          lcd.clear();
          
                              
         }
        else if (btt2.pressed == true) 
        {
          opc = 2; //2 para gsm
          btt1.pressed = false;
          btt2.pressed = false;
          lcd.clear();
          lcd.print("Configurando GSM");
          delay(10);
          Serial.println("Confiruando GSM");
          setup_gsm();
          delay(10); 
          lcd.clear();         
         }                 
    }

    
    current_time= millis();
    //Muestra en LCD
    if(current_time - last_reading_time >= reading_period){
      humidity_value = analogRead(humidity_pin);
      Serial.println(humidity_value);
      show_to_lcd(2);            
      last_reading_time = millis();     
    }
    //Envía a la pagina web
    if (current_time - last_sending_time >= sending_period){      
      if(opc == 1)
      { 
        sending_data_wifi();
      }      
      else if (opc == 2)
      {
        sending_data_gsm();
      }
      Serial.println("Humedad: "); 
      delay(5); 
      Serial.print(humidity_value);
      last_sending_time = millis();
      //lcd.clear();
      //lcd.print("Durimiendo..");
      //delay(2000);
      //lcd.clear();
      //lcd.noBacklight();
      //Serial.flush(); 
      //esp_deep_sleep_start();      
    }
 

}
