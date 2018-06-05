//Porta do Arduino onde o pino TX do Módulo GSM esta conectado
//#define TX_PIN 10
 
//Porta do Arduino onde o pino RX do Módulo GSM esta conectado
//#define RX_PIN 11
 
//Cria comunicacao serial via software nas portas digitais definidas acima
//SoftwareSerial serialGSM(TX_PIN, RX_PIN);

#include <HardwareSerial.h>

HardwareSerial Serial1(1);
#define SERIAL1_RXPIN 4
#define SERIAL1_TXPIN 2
HardwareSerial Serial2(2);

void setup() {
  //Begin serial comunication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(115200);
  while(!Serial);

  Serial1.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);
   
  //Inicia comunicacao serial com o GSM
//  Serial2.listen();
  Serial2.begin(115200);
  delay(1000);
   
  Serial.println("Setup Completo!");
}
 
void loop() {
  //Le as mensagens vindas do GSM para escrevê-las no monitor serial

  if(Serial2.available()){
    Serial.write(Serial2.read());
  }

  if(Serial1.available()){
    Serial.write(Serial1.read());
  }

  /*if(Serial.available()){
    Serial1.write(Serial.read());
  }*/

  //Le as mensagens vindas do monitor serial para copiá-las para o GSM
  if(Serial.available()){    
    Serial2.write(Serial.read());
  }
}



