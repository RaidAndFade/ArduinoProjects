#include <OneWire.h> 
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;
int freeMemory() {
  int free_memory;

  if((int)__brkval == 0)
     free_memory = ((int)&free_memory) - ((int)&__bss_end);
  else
    free_memory = ((int)&free_memory) - ((int)__brkval);

  return free_memory;
}

String WIFI_SSID = "[]";
String WIFI_PWD = "[]";
String PROG_PORT = "5050";

void setup() {
     Serial.begin(115200);
     while (!Serial){}
     Serial1.begin(115200);
     while (!Serial1){}
     sensors.begin();

     //Reset, and set correct mode for esp8266
     Serial.print("Preparing");
     sendSetupCommand("AT+RST","ready"); //restart
     sendSetupCommand("AT+CWMODE=1","OK"); //client mode
     sendSetupCommand("AT+CWQAP","OK"); //quit ap
     sendSetupCommand("AT+CWJAP=\""+WIFI_SSID+"\",\""+WIFI_PWD+"\"","OK"); //join ap
     sendSetupCommand("AT+CIPMUX=1","OK"); //multiconnection mode
     sendSetupCommand("AT+CIPSERVER=1,"+PROG_PORT,"OK"); //server on 5050

     String info[2][4];
     getWifiInfo(info);
     
     Serial.println("WIFI: "+ info[0][0] + " (" + info[0][1] + ") CHAN " + info[0][2] + " @ " + info[0][3]);
     Serial.println("IP: "+ info[1][0]);
     Serial.println("MAC: "+ info[1][1]);
     
     Serial.println("Running server @ "+info[1][0]+":"+PROG_PORT);
     
     Serial.println("Ready.");
}

void getWifiInfo(String buf[2][4]){
     char c;
     //GET SSID
     Serial1.println("AT+CWJAP?");
     waitForStringS1("+CWJAP:");
     buf[0][0] = readS1Until('"',1);
     buf[0][1] = readS1Until('"',2);
     buf[0][2] = readS1Until(',',1);
     buf[0][3] = readS1Until('\n');
     
     
     //GET IP
     Serial1.println("AT+CIFSR");
     waitForStringS1("+CIFSR:STAIP,\"");
     buf[1][0] = readS1Until('"');
     Serial.print(".");
     
     
     waitForStringS1("+CIFSR:STAMAC,\"");
     buf[1][1] = readS1Until('"');
     Serial.println(".");
     
     waitForStringS1("OK");
}

void sendSetupCommand(String cmd, String resp){
     Serial1.println(cmd);
     waitForStringS1(resp);
     Serial.print(".");
}

String readS1Until(char e,int pad){
  for(int i=0;i<pad;i++){
    while (Serial1.available() == 0) {}
    Serial1.read();
  }
  return readS1Until(e);
}
String readS1Until(char e){
   String o;
   char c;
   while(true){
      while (Serial1.available() == 0) {}
      c=Serial1.read();
      if(c==e) break;
      o+=c;
   }
   return o;
}

void waitForStringS1(String s){
  int i = 0;
  int l = s.length();
  while(i<l){
    while(Serial1.available()==0){}
    char c = Serial1.read();
    if(c == s[i]){
      i++;
    }else{
      i=0;
    }
  }
}

String waitForAnyStringS1(String s[]) {
  int sl = sizeof(s);
  int i[sl];
  if (sl == 0)
    return "";
  int l = s[0].length();
  for (int x = 0; x < sl; x++) {
    i[x] = 0;
    if (s[x].length() > l)
      l = s[x].length();
  }
  while (true) {
    while (Serial1.available() == 0) {}
    
    char c = Serial1.read();
    for (int x = 0; x < sl; x++) {
      if (c == s[x][i[x]]) {
        i[x]++;
        if (s[x].length() == i[x]) {
          return s[x];
        }
      } else {
        i[x] = 0;
      }
    }
  }
}

void respond(int i, String resp) {
  int l = resp.length();
  Serial1.print("AT+CIPSEND=");
  Serial1.print(i,DEC);
  Serial1.print(",");
  Serial1.println(l,DEC);
  waitForStringS1(">");
  Serial1.println(resp);
}

void doFunc(int i, String cmd) {
   if(cmd=="i"){//interactive kek
      
   }

   if(cmd.startsWith("uptime")){
    respond(i,String(millis()));
   }

   if(cmd.startsWith("light")){
     respond(i,String(analogRead(0)));    
   }

   if(cmd.startsWith("mem")){
     respond(i,String(freeMemory()));
   }

   if(cmd.startsWith("ping")){
     Serial1.println("AT+PING=\"8.8.8.8\"");
     waitForStringS1("\"");
     String f[] = {"+","ERROR"};
     String d = waitForAnyStringS1(f);
     if(d=="+"){
      int ping = Serial1.parseInt();
      waitForStringS1("OK");
      respond(i,String(ping));
     }else{
      respond(i,"-1");
     }
   }

   if(cmd.startsWith("temp")){
     sensors.requestTemperatures();
     respond(i,String(sensors.getTempCByIndex(0)));
   }

   if(cmd.startsWith("cmd:")){
    
   }

   if(cmd.startsWith("laser")){
     int lI = cmd.indexOf(";");
     int nI = cmd.indexOf(";",lI+1);
     int v = min(255,max(0,cmd.substring(lI+1,nI).toInt()));
     pinMode(11,OUTPUT);
     analogWrite(11,v);
     respond(i,"1");
   }

   if(cmd.startsWith("rgb")){
     int lI = cmd.indexOf(";");
     for(int i=0;i<3;i++){
       int nI = cmd.indexOf(";",lI+1);
       int v = min(255,max(0,cmd.substring(lI+1,nI).toInt()));
       lI = nI;
       //Serial.println(v);
       pinMode(8+i,OUTPUT);
       analogWrite(8+i,v); 
     }
     respond(i,"1");
   }
}

void cmdmode(){
     waitForStringS1("IPD,"); //or connect. or whatever.
     int id = Serial1.parseInt();
     Serial1.read();
     int ln = Serial1.parseInt();
     Serial1.read();
     String cmd;

     Serial.print("Command from ");
     Serial.print(id);
     Serial.print("(");
     Serial.print(ln);
     Serial.print(")");
     Serial.print(": ");
     while(ln-->0){
        if(!Serial1.available()){
          delay(50);
          continue;
        }
        char c = Serial1.read();
        cmd += c;
        delay(10);
        Serial.print(c);
     }
     Serial.println();
     cmd.trim();
     Serial.println(cmd);
     doFunc(id,cmd);

     return;
}

void intermode(){
    while (Serial1.available() > 0) {
           String res;
           res += (char)Serial1.read();
           if(res[0] == 43 && false){
              res = "";
              int r = 0;
              Serial.println("YEE");
              if(r == 3 && res == "IPD,"){
                  int id = Serial1.read();
                  Serial.println("YEET");
                  Serial.println(id,DEC);
              }else{
                  for(int i=0;i<r;i++){
                     Serial.print("I received: ");
                     Serial.println(res[i],DEC);
                  }
              }
           }
           Serial.print(res);
     }

     if (Serial.available() > 0) {
         Serial1.write(Serial.read());
     }
}

void loop() {
     cmdmode();
     //intermode();
}
