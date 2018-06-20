#include <WiFi.h>
#include <TinyGPS.h>
#include <IOXhop_FirebaseESP32.h>

HardwareSerial Serial1(1);
#define SERIAL1_RXPIN 4
#define SERIAL1_TXPIN 2
HardwareSerial Serial2(2);

#define FIREBASE_HOST "esp32-a7.firebaseio.com"
#define FIREBASE_AUTH "p0tUa8mAysVEtLGw1Vk6sCACnpGG2aJo0VlTZi1F"
#define WIFI_SSID "WiFi_SMG"
#define WIFI_PASSWORD "faculdadesmg"

#define S1debug true
#define S_debug true

#define pinBtnOn 18
#define pinBtnOff 5
#define ledVerde 22
#define ledVermelho 23
#define pinJump 21

#define numeroCall "044991797155"

TinyGPS gps;

bool alarme = true;
bool alarmeAnterior = false;
bool valAlarme;
bool valStatus;
bool jumpAlarme;

bool callStatus = false;
bool menssageStatus = false;

byte alarmeFull = 1;
byte alarmeHalf = 2;
byte notifica = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);
  Serial2.begin(115200);

  pinMode(pinBtnOn, INPUT_PULLUP);
  pinMode(pinBtnOff, INPUT_PULLUP);
  pinMode(pinJump, INPUT_PULLUP);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledVermelho, OUTPUT);

  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  Firebase.setString("alarme", (String)alarme);
  Firebase.setString("status", (String)alarme);

  xTaskCreatePinnedToCore(coreTaskAlarme,"coreTaskAlarme",10000,NULL,1,NULL,0);
  delay(500);
  xTaskCreatePinnedToCore(coreTaskNotifica,"coreTaskNotifica",10000,NULL,1,NULL,0);
  delay(500);
  xTaskCreatePinnedToCore(coreTaskDataBase,"coreTaskDataBase",10000,NULL,6,NULL,1);
  delay(500);

  configuraGSM();
  leGPS();

  Serial.println("Sketch Iniciado!");
}

void coreTaskAlarme(void * pvParameters){
  while(true){
    estadoAlarme(alarme);

    btn();

    jumpAlarme = digitalRead(pinJump);
  
    if(alarme != alarmeAnterior){
      if(alarme && jumpAlarme){
        acionaAlarme(alarmeHalf);
      }
    }

    if(alarme == alarmeAnterior){
      if(alarme && jumpAlarme){ 
        acionaAlarme(alarmeFull);
      }  
    }
  }
}

void estadoAlarme(bool alarme)
{
  if(alarme)
  {
      digitalWrite(ledVerde, LOW);
      digitalWrite(ledVermelho, HIGH); 
  }
  else
  {
      digitalWrite(ledVerde, HIGH);
      digitalWrite(ledVermelho, LOW); 
  }
}

void acionaAlarme(byte tipoAlarme)
{
  if(tipoAlarme == alarmeFull)
  {
    notifica = alarmeFull;
    
    digitalWrite(ledVerde, LOW);
    while(alarme)
    {
      btn();
      digitalWrite(ledVermelho, HIGH);
      delay(80);
      digitalWrite(ledVermelho, LOW);
      delay(80);
    }
  }

  if(tipoAlarme == alarmeHalf)
  {
    notifica = alarmeHalf;
    digitalWrite(ledVerde, LOW);
    while(alarme)
     {
      btn();
      digitalWrite(ledVermelho, HIGH);
      delay(200);
      digitalWrite(ledVermelho, LOW);
      delay(200);  
    }
  }
}

void btn(){
  bool btnOn = !digitalRead(pinBtnOn);
  if(btnOn && alarme == false){
    alarme = true;
    callStatus = menssageStatus = alarmeAnterior = !alarme;
  }
      
  bool btnOff = !digitalRead(pinBtnOff);
  if(btnOff && alarme){
    alarme = false;
    alarmeAnterior = !alarme;
  } 
}

void coreTaskNotifica(void * pvParameters){
  while(true){
    if(notifica == alarmeHalf){
      if(!menssageStatus == true)
      {
        enviaSMS(numeroCall, "Atencao, porta aberta!");
        menssageStatus = true;
        notifica = 0;
      }
    }
  
    if(notifica == alarmeFull){
      if(!menssageStatus == true){
        Serial.println("Enviando SMS de Resposta.");  
        leGPS();
  
        float flat, flon;
        unsigned long age;
  
        gps.f_get_position(&flat, &flon, &age);
  
        if ( (flat == TinyGPS::GPS_INVALID_F_ANGLE) || (flon == TinyGPS::GPS_INVALID_F_ANGLE) ) {
           enviaSMS(numeroCall, "GPS Sem Sinal !!!");
           menssageStatus = true;
        } else {
           String urlMapa = "Local Identificado: https://maps.google.com/maps/?&z=10&q=";
           urlMapa += String(flat,6);
           urlMapa += ",";
           urlMapa += String(flon,6);
             
           enviaSMS(numeroCall, urlMapa);
           menssageStatus = true;
        }
      }

      if(!callStatus) {
        Serial.println("Efetuando Ligacao..."); 
        fazLigacao(numeroCall);
        callStatus = true;
      }
      
      notifica = 0;
    } 
  }
}

void enviaSMS(String telefone, String mensagem) {  
  SendData("AT+CMGS=" + telefone ,80,S1debug);//send sms message, be careful need to add a country code before the cellphone number
  SendData(mensagem,80,S1debug);//the content of the message
  Serial2.println((char)26);//the ASCII code of the ctrl+z is 26
}

void fazLigacao(String telefone) {
  SendData("ATH",80,S1debug);
  SendData("ATD" + telefone + ";",80,S1debug);// "ATD+86137xxxxxxxx"dial the number
}

void coreTaskDataBase(void * pvParameters){
  while(true){
    getDataBase();
    setDataBase();
  }
}

void setDataBase(){
  if(alarme != alarmeAnterior){
    Firebase.setString("alarme", (String)alarme);
    Firebase.setString("status", (String)alarme);
    alarmeAnterior = alarme;
  }

 static unsigned long delayLeGPS = millis();

  if ( (millis() - delayLeGPS) > 10000 ) {
    leGPS();
  
    float flat, flon;
    unsigned long age;
      
    gps.f_get_position(&flat, &flon, &age);
                
    String latitude = String(flat,6);
    String longitude = String(flon,6);
            
    Firebase.setString("latitude", latitude);
    Firebase.setString("longitude", longitude);
  
    //Serial.println(latitude + " " + longitude);
  
    delayLeGPS = millis();
  }  
}

void getDataBase(){
  valAlarme = Firebase.getString("alarme").toInt();
  valStatus = Firebase.getString("status").toInt();

  if(valAlarme != valStatus){
    alarme = valAlarme;
    alarmeAnterior = !alarme;
    Firebase.setString("status", (String)alarme);
    if(!valAlarme) callStatus = menssageStatus = valAlarme; 
  }
}

void leGPS() {
  unsigned long delayGPS = millis();

  bool lido = false;
  while ( (millis() - delayGPS) < 500 ) { 
    while (Serial1.available()) {
      char cIn = Serial1.read(); 
      lido = gps.encode(cIn); 
    }

    if (lido) { 
       float flat, flon;
       unsigned long age;
    
       gps.f_get_position(&flat, &flon, &age);
    
       String urlMapa = "Local Identificado: https://maps.google.com/maps/?&z=10&q=";
       urlMapa += String(flat,6);
       urlMapa += ",";
       urlMapa += String(flon,6);
       Serial.println(urlMapa);

       break; 
   }
  }   
}

void loop() {}

void configuraGSM(){
  SendData("AT+CMGF=1",100,S1debug);
  SendData("AT+CNMI=2,2,0,0,0",80,S1debug);
  SendData("ATX4",80,S1debug);
  SendData("AT+COLP=1",80,S1debug);
  SendData("AT+GPS=1",80,S1debug);
}

void SendData(String command, const int timeout, boolean debug){
  String response = "";    
  Serial2.println(command); 
  delay(5);
  if(debug){
    long int time = millis();   
    while( (time+timeout) > millis()){
      while(Serial2.available()){       
        response += char(Serial2.read());
      }  
    }    
    Serial.print(response);
  }    
}

