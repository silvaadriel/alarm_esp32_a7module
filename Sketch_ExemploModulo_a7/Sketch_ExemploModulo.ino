HardwareSerial Serial1(2);

#define S1debug true
#define S_debug true


void setup()
{
  Serial1.begin(115200);   // the GPRS baud rate   
  Serial.begin(115200);    // the GPRS baud rate
}
 
void loop()
{
/*
 * After start up the program, you can using terminal to connect the serial of gprs shield,
 * If you input 't' in the terminal, the program will execute SendTextMessage(), it will show how to send a sms message,
 * If input 'd' in the terminal, it will execute DialVoiceCall(), etc.
 */
  delay(500);
  if (Serial.available()){
    switch(Serial.read()){
     case 't':
       SendTextMessage();
       break;
     case 'd':
       DialVoiceCall();
       break;
     case 'h':
       SubmitHttpRequest();
       break;
     case 'q':
       GetSignalQuality();
       break;
     case 'a':
       configuraGSM();
       break;
   } 
  }
   RecData();
}
 
/*
 * SendTextMessage()
 * This function is to send a sms message
 */
void SendTextMessage()
{ 
  SendData("",2000,S1debug);
  SendData("AT+CMGF=1",2000,S1debug);//Because we want to send the SMS in text mode
  delay(100);
  SendData("AT+CMGS=+5544991797155",2000,S1debug);//send sms message, be careful need to add a country code before the cellphone number
  delay(100);
  SendData("GSM test message!",2000,S1debug);//the content of the message
  delay(100);
  Serial1.println((char)26);//the ASCII code of the ctrl+z is 26
  delay(100);
}
 
/*
 * DialVoiceCall
 * This function is to dial a voice call
 */
void DialVoiceCall()
{
   SendData("AT+SNFS=0",2000,S1debug);
   SendData("ATH",2000,S1debug);
   delay(100);
   SendData("ATD044991797155;",2000,S1debug);// "ATD+86137xxxxxxxx"dial the number
   //SendData("ATH",2000,S1debug);
   delay(1000);

}
 
/*
 * SubmitHttpRequest()
 * This function is submit a HTTP request
 * attention:the time of delay is very important, it must be set enough 
 */
void SubmitHttpRequest(){
   SendData("AT+CREG?",1000,S1debug); //Query network registration
   delay(100);
   
   SendData("AT+CGATT=1",2000,S1debug);
   delay(100);
 
   SendData("AT+CGDCONT=1,\"IP\",\"CMNET\"",2000,S1debug);//setting PDP parameter 
   delay(100);
 
   SendData("AT+CGACT=1,1",2000,S1debug); //Activate PDP, open Internet service
   delay(100);
 
   SendData("AT+CIPSTART=\"TCP\",\"www.google.com\",80",5000,S1debug); //Establish TCP connection
   delay(100);
 
   SendData("AT+CIPSEND=10,\"asdfg12345\"",2000,S1debug); //Send string "asdfg12345" and the length of string is 10
   delay(100); 
   
   SendData("AT+CIPCLOSE",2000,S1debug);     //Close TCP
   delay(100); 
}

void GetSignalQuality(){
     Serial_Print("Getting the sinal quality...",S_debug);
     SendData("AT+CSQ",1000,S1debug);
     Serial_Print("Tips:+CSQ: XX,QQ : It means the Signal Quality poorly when the XX less then 10!",S_debug);
}

void SendData(String command, const int timeout, boolean debug){
    String response = "";    
    Serial1.println(command); 
    delay(5);
    if(debug){
    long int time = millis();   
    while( (time+timeout) > millis()){
      while(Serial1.available()){       
        response += char(Serial1.read());
      }  
    }    
      Serial.print(response);
    }    
}

void Serial_Print(String data, boolean debug){
  if(debug){
    Serial.println(data);
  }
}

void RecData(){
    String response = "";    
    while(Serial1.available()){       
        response += char(Serial1.read());
      }    
      Serial.print(response);    
}

void configuraGSM(){
    SendData("AT+CMGF=1",2000,S1debug);
    delay(200);
    SendData("AT+CNMI=2,2,0,0,0",2000,S1debug);
    delay(200);
    SendData("ATX4",2000,S1debug);
    delay(200);  
    SendData("AT+COLP=1",2000,S1debug);
    delay(200);
}


