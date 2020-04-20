#include <stdio.h> /* Prototipos de intrada/salida */
#include "WiFi.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>


#define TINY_GSM_MODEM_SIM800 // definição do modem usado (SIM800L)
#define TINY_GSM_RX_BUFFER   1024
#include <TinyGsmClient.h> // biblioteca com comandos GSM

//---------GSM-----------
HardwareSerial SerialGSM(2); //Uso de modulo UART2 para conexion con el modulo GSM
TinyGsm modemGSM(SerialGSM); // Objeto TinyGsm
const int BAUD_RATE = 9600; // Velocidad de comunicacion
const int RX_PIN = 16, TX_PIN = 17; //Pines para conectar al modulo GSM

//Credenciales para conectar datos de SIM. Valido para ENTEL 
const char apn[]  = "imovil.entelpcs.cl";
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


unsigned int led = 33;

void show_to_lcd(){   
    
      char data[10];
      //lcd.clear();
      //me paro en la primera fila
      lcd.setCursor(0,0);
      lcd.print("             ");
      lcd.setCursor(0,0);    
      
      sprintf(data, "Hum: %d", humidity_value);
      lcd.print(data);   
           
  }

  
void setup_gsm(){
  Serial.println("Starting...");
  SerialGSM.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN, false);
  delay(3000);
  Serial.println(modemGSM.getModemInfo());
  delay(100);
  if (!modemGSM.restart()) {
  // if (!modem.init()) {
    Serial.println(F(" Restarting"));
    
    delay(100);
    return;
  }

  if (!modemGSM.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println("Connection failed");
    }
    else{
      Serial.println("Connected !!!");
    }  
  }


void sending_data(){

  if (modemGSM.gprsConnect(apn, gprsUser, gprsPass)) { //Check WiFi connection status
 
    StaticJsonBuffer<400> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[400];

    jsonObj["measurement_point_id"] = String(measurement_point_id);
    jsonObj["token"] = String(TOKEN);
    jsonObj["moisture"] = String(humidity_value);
    jsonObj["temperature"] = "210";
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
 
    //Serial.println("Error in WiFi connection");
    Serial.println("Error in Data connection");
 
   }

  
}


void setup() {
  // put your setup code here, to run once:
  WiFi.mode(WIFI_STA); //Configuro el ESP32 como station mode para que se pueda conectar a alguna red
  WiFi.disconnect(); 

  Serial.begin(BAUD_RATE);
  WiFi.begin(WIFI_SSID, PASSWORD);
  pinMode(humidity_pin, INPUT);
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  delay(10);

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

  if(WiFi.status()== WL_CONNECTED){
    digitalWrite(led, HIGH);
  }

    
  
  lcd.init();                                                     //INICIO LCD
  delay(10);
  
  lcd.backlight();                                                //Enciendo la luz trasera del LCD
  lcd.setCursor(0,0);
  lcd.print("Sin medida");
  setup_gsm(); 
  WiFi.disconnect(); 
  Serial.println("Wifi disconnected");
}

void loop() {
  // put your main code here, to run repeatedly:
  lcd.print("loop");  

  while(!Serial){
    //Me quedo aquí mientras no haya comunicación serial 
    lcd.print("Esperando comunicacion serial");   
    
    }

    if (!modemGSM.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println("Connection failed");
    }
    else{
      Serial.println("Connected !!!");
    }  
    

    current_time= millis();
    //Muestra en LCD
    if(current_time - last_reading_time >= reading_period){
      humidity_value = analogRead(humidity_pin);
      Serial.println(humidity_value);
      show_to_lcd();        
      last_reading_time = millis();            
      
    }
    //Envía a la pagina web
    if (current_time - last_sending_time >= sending_period){
      
      sending_data();
      Serial.println("Humedad: "); 
      delay(5); 
      Serial.print(humidity_value);
      last_sending_time = millis();
      
    }
 

}
