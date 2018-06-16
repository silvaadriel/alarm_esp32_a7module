#include <TinyGPS.h>
#include <WiFi.h>
#include <IOXhop_FirebaseESP32.h>

#define FIREBASE_HOST "esp32-a7.firebaseio.com"
#define FIREBASE_AUTH "p0tUa8mAysVEtLGw1Vk6sCACnpGG2aJo0VlTZi1F"
#define WIFI_SSID "WiFi_SMG"
#define WIFI_PASSWORD "faculdadesmg"

HardwareSerial Serial1(1);
#define SERIAL1_RXPIN 4
#define SERIAL1_TXPIN 2
HardwareSerial Serial2(2);

TinyGPS gps;

byte pinLed[] = {23, 22};
byte vermelho = 0;
byte verde = 1;
byte pinJump = 21;
byte alrFull = 1;
byte alrHalf = 2;

bool alr = true;
bool alrAnt = true;
bool jumpAlr;
bool jumpAlrAnt = false;
bool btnAntOn;
bool btnAntOff;
bool btnAtOn;
bool btnAtOff;
bool val;
bool valAnt = true;

bool temSMS = false;
String telefoneSMS;
String dataHoraSMS;
String mensagemSMS;
String comandoGSM = "";
String ultimoGSM = "";

#define S1debug true
#define S_debug true

#define senhaGsm "1234"
#define ligaLedVerde "LG"
#define desligaLedVerde "DG"
#define ligaLedVermelho "LR"
#define desligaLedVermelho "DR"
#define pinBtnOff 5
#define pinBtnOn 18
#define pinBotaoCall 12
#define numeroCall "044991797155" 

bool callStatus = false;
bool menssageStatus = false;

bool btn(byte tipoBtn, bool estadoAnt);
void acionaAlarme(bool estadoAlr, byte tipoAlr);
void estadoAlarme(bool alr);
void leGSM();
void enviaSMS(String telefone, String mensagem);
void fazLigacao(String telefone);
void configuraGSM();
//void leGPS();

void setup() {

  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);
  Serial2.begin(115200);
 // serialGSM.begin(9600);

  pinMode(pinBotaoCall, INPUT);
  pinMode(pinBtnOn, INPUT_PULLUP);
  pinMode(pinBtnOff, INPUT_PULLUP);
  pinMode(pinJump, INPUT_PULLUP);
  btnAntOn = digitalRead(pinBtnOn);
  btnAntOff = digitalRead(pinBtnOff);
  pinMode(pinLed[vermelho], OUTPUT);
  pinMode(pinLed[verde], OUTPUT);

  
    // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(pinLed[vermelho], HIGH);
    delay(500);
    digitalWrite(pinLed[vermelho], LOW);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  
  Serial.println("Sketch Iniciado!");
  Firebase.setBool("alr", true);
  configuraGSM();
  leGPS();
}

void loop() {
  static unsigned long delayLeGPS = millis();

  if ( (millis() - delayLeGPS) > 5000 ) {
     leGPS();     
     delayLeGPS = millis(); 
  }
  
  
  jumpAlr = digitalRead(pinJump);
  
  estadoAlarme(alr);

  alr = btn(pinBtnOn, alr);
  
  alr = btn(pinBtnOff, alr);
  

  if(alr != alrAnt)
  {
    if((alr == true) && ((jumpAlr == HIGH) || (jumpAlrAnt == false)))
    {
      digitalWrite(pinLed[verde], LOW);
      digitalWrite(pinLed[vermelho], HIGH);
      alr = btn(pinBtnOff, alr);
      delay(150);
      digitalWrite(pinLed[vermelho], LOW);
      alr = btn(pinBtnOff, alr);
      delay(150);

      if (ultimoGSM.indexOf("NO CARRIER") > -1) 
      { 
        ultimoGSM = "";
        menssageStatus = false;
      }

      if(!menssageStatus == true)
      {
        enviaSMS(numeroCall, "Atencao, porta aberta!");
        menssageStatus = true;
      }
      jumpAlr = false;
    }
    
  }

  if((alr == true) && ((jumpAlr == true) || (jumpAlrAnt == true)))
  { 
    jumpAlrAnt = alr;

    digitalWrite(pinLed[vermelho], HIGH);
    alr = btn(pinBtnOff, alr);
    delay(80);
    digitalWrite(pinLed[vermelho], LOW);
    alr = btn(pinBtnOff, alr);
    //delay(80);

    if(!menssageStatus == true)
    {
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

    delay(80);
    
    if (jumpAlr == true && !callStatus && alr == true) {
     Serial.println("Efetuando Ligacao..."); 
     fazLigacao(numeroCall);
     callStatus = true;
    }

}
  
  leGSM();

  if (comandoGSM != "") {
      Serial.println(comandoGSM);
      ultimoGSM = comandoGSM;
      comandoGSM = "";
  }

  if (temSMS) {

     Serial.println("Chegou Mensagem!!");
     Serial.println();
    
     Serial.print("Remetente: ");  
     Serial.println(telefoneSMS);
     Serial.println();
    
     Serial.print("Data/Hora: ");  
     Serial.println(dataHoraSMS);
     Serial.println();
    
     Serial.println("Mensagem:");  
     Serial.println(mensagemSMS);
     Serial.println();
      
     mensagemSMS.trim();      
     if ( mensagemSMS == senhaGsm ) {
        Serial.println("Enviando SMS de Resposta.");  
         enviaSMS(telefoneSMS, "SMS Recebido e Senha OK!");
     }
     
     if ( mensagemSMS == ligaLedVerde ) {
        Serial.println("Ligando led verde.");  
        digitalWrite(pinLed[verde], HIGH);
     }
     
     if ( mensagemSMS == ligaLedVermelho ) {
        Serial.println("Ligando led vermelho.");  
        digitalWrite(pinLed[vermelho], HIGH);
     }

     if ( mensagemSMS == desligaLedVerde ) {
        Serial.println("Desligando led verde.");  
        digitalWrite(pinLed[verde], LOW);
     }

     if ( mensagemSMS == desligaLedVermelho ) {
        Serial.println("Desigando led vermelho.");  
        digitalWrite(pinLed[vermelho], LOW);
     }
     
     temSMS = false;
  }
/*
  if (digitalRead(pinBotaoCall) && !callStatus) {
     Serial.println("Afetuando Ligacao..."); 
     fazLigacao(numeroCall);
     callStatus = true;
  }
*/
  if (ultimoGSM.indexOf("+COLP:") > -1) {
     Serial.println("LIGACAO EM ANDAMENTO");
     ultimoGSM = "";                
  }
       
  if (ultimoGSM.indexOf("NO CARRIER") > -1) {
     Serial.println("LIGACAO TERMINADA");
     ultimoGSM = "";
     callStatus = false;
  }
       
  if (ultimoGSM.indexOf("BUSY") > -1) {
     Serial.println("LINHA/NUMERO OCUPADO");
     ultimoGSM = "";
     callStatus = false;
  }

  if (ultimoGSM.indexOf("NO DIALTONE") > -1) {
     Serial.println("SEM LINHA");
     ultimoGSM = "";
     callStatus = false;
  }
       
  if (ultimoGSM.indexOf("NO ANSWER") > -1) {
     Serial.println("NAO ATENDE");
     ultimoGSM = "";
     callStatus = false;
  }
  
}

void estadoAlarme(bool alr)
{  
  if(alr == true)
  {
      digitalWrite(pinLed[verde], LOW);
      digitalWrite(pinLed[vermelho], HIGH); 
  }
  else
  {
      digitalWrite(pinLed[verde], HIGH);
      digitalWrite(pinLed[vermelho], LOW); 
  }
}

void acionaAlarme(bool estadoAlr, byte tipoAlr)
{
  if((estadoAlr == true) && (tipoAlr == alrFull))
  {
    //fazLigacao(numeroCall);
    digitalWrite(pinLed[verde], LOW);
    while(alr == true)
    {
      alrAnt = alr = btn(pinBtnOff, alr);
      digitalWrite(pinLed[vermelho], HIGH);
      delay(100);
      digitalWrite(pinLed[vermelho], LOW);
      delay(100);
    }
  }

  if((estadoAlr == true) && (tipoAlr == alrHalf))
  {
    alr = true;
    enviaSMS(numeroCall, "Atençao, porta aberta!");
    digitalWrite(pinLed[verde], LOW);
    while(alr == true)
     {
      alrAnt = alr = btn(pinBtnOff, alr);
      digitalWrite(pinLed[vermelho], HIGH);
      delay(250);
      digitalWrite(pinLed[vermelho], LOW);
      delay(250);  
    }
  }
}

bool btn(byte tipoBtn, bool estadoAnt)
{
  bool estadoAt = estadoAnt;

  val = Firebase.getBool("alr");

    alr = val;
  
  if(tipoBtn == pinBtnOn)
  {
    btnAtOn = !digitalRead(pinBtnOn);
    if((btnAtOn == HIGH) && (btnAntOn == LOW) && (jumpAlr == HIGH))
    {
      callStatus = menssageStatus = alrAnt = false;
      estadoAt = true;
      Firebase.setBool("alr", true);
    }
    if((btnAtOn == HIGH) && (btnAntOn == LOW) && (jumpAlr == LOW))
    {
      callStatus = menssageStatus = false;
      alrAnt = true;
      estadoAt = true;
      Firebase.setBool("alr", true);
    }
    btnAntOn = btnAtOn;
  }

  if(tipoBtn == pinBtnOff)
  {
    btnAtOff = !digitalRead(pinBtnOff);
    if((btnAtOff == HIGH) && (btnAntOff == LOW))
    {
      callStatus = alrAnt = false;
      jumpAlrAnt = estadoAt = false;
      Firebase.setBool("alr", false);
    }
    btnAntOff = btnAtOff;
  }

  valAnt = val;

  if(estadoAt == estadoAnt)
  {
    return estadoAnt;
  }
  else
  {
    return estadoAt;
  }
  

}

void leGPS() {
unsigned long delayGPS = millis();

   //serialGPS.listen();
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

void leGSM()
{
  static String textoRec = "";
  static unsigned long delay1 = 0;
  static int count=0;  
  static unsigned char buffer[64];

  if (Serial2.available()) {            
 
     while(Serial2.available()) {         
   
        buffer[count++] = Serial2.read();     
        if(count == 64)break;
     }

     textoRec += (char*)buffer;
     delay1   = millis();
     
     for (int i=0; i<count; i++) {
         buffer[i]=NULL;
     } 
     count = 0;                       
  }


  if ( ((millis() - delay1) > 100) || textoRec != "" ) {

     if ( textoRec.substring(2,7) == "+CMT:" ) {
        temSMS = true;
     }

     if (temSMS) {
            
        telefoneSMS = "";
        dataHoraSMS = "";
        mensagemSMS = "";

        byte linha = 0;  
        byte aspas = 0;
        for (int nL=1; nL < textoRec.length(); nL++) {

            if (textoRec.charAt(nL) == '"') {
               aspas++;
               continue;
            }                        
          
            if ( (linha == 1) && (aspas == 1) ) {
               telefoneSMS += textoRec.charAt(nL);
            }

            if ( (linha == 1) && (aspas == 5) ) {
               dataHoraSMS += textoRec.charAt(nL);
            }

            if ( linha == 2 ) {
               mensagemSMS += textoRec.charAt(nL);
            }

            if (textoRec.substring(nL - 1, nL + 1) == "\r\n") {
               linha++;
            }
        }
     } else {
       comandoGSM = textoRec;
     }
     
     textoRec = "";  
  }     
}


void enviaSMS(String telefone, String mensagem) {
 /* Serial2.print("AT+CMGS=\"" + telefone + "\"\n");
  delay(200);
  Serial2.print(mensagem + "\n");
  Serial2.print((char)26); 
*/  
  SendData("AT+CMGS=" + telefone ,80,S1debug);//send sms message, be careful need to add a country code before the cellphone number
  //delay(100);
  SendData(mensagem,80,S1debug);//the content of the message
  //delay(100);
  Serial2.println((char)26);//the ASCII code of the ctrl+z is 26
  //delay(100);
}

void fazLigacao(String telefone) {
/*  Serial2.println("ATH\n");
  Serial2.print((char)26); 
  Serial2.println("ATD" + telefone + ";\n");
  Serial2.print((char)26); 
  SendData("AT+SNFS=0",2000,S1debug);*/
  //SendData("ATH",80,S1debug);
  //delay(100);
  SendData("ATD" + telefone + ";",80,S1debug);// "ATD+86137xxxxxxxx"dial the number
  //delay(100);
}


void configuraGSM(){
    SendData("AT+CMGF=1",100,S1debug);
   // delay(30);
    SendData("AT+CNMI=2,2,0,0,0",80,S1debug);
   // delay(30);
    SendData("ATX4",80,S1debug);
    //delay(50);  
    SendData("AT+COLP=1",80,S1debug);
   // delay(50);
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

