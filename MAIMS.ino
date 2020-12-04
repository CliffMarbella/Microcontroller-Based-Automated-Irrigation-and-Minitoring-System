
#include <SoftwareSerial.h>

#include <Wire.h>

#include <LiquidCrystal_I2C.h>

#define REMOTEXY_MODE__ESP8266_HARDSERIAL_POINT

#include <RemoteXY.h>

// RemoteXY connection settings 
#define REMOTEXY_SERIAL Serial
#define REMOTEXY_SERIAL_SPEED 115200
#define REMOTEXY_WIFI_SSID "MAIMS"
#define REMOTEXY_WIFI_PASSWORD "12345678"
#define REMOTEXY_SERVER_PORT 6377


// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =
  { 255,0,0,21,0,12,1,8,13,5,
  130,1,33,50,28,47,0,13,130,1,
  2,14,29,83,0,13,66,1,4,26,
  14,69,0,188,26,129,0,19,27,11,
  4,0,6,49,48,48,37,0,129,0,
  19,54,8,4,0,50,54,48,37,0,
  129,0,19,41,8,4,0,131,56,48,
  37,0,129,0,19,68,8,4,0,1,
  52,48,37,0,129,0,19,82,8,4,
  0,24,50,48,37,0,65,6,49,64,
  9,9,0,65,6,49,86,9,9,0,
  65,6,49,75,9,9,0,129,0,37,
  77,9,4,0,24,71,83,77,0,129,
  0,36,88,11,4,0,24,80,117,109,
  112,0,129,0,33,59,0,6,0,16,
  0,129,0,37,67,8,4,0,24,69,
  83,80,0,131,1,19,3,30,8,1,
  132,24,77,65,73,77,83,0,130,1,
  33,14,28,34,0,13,129,0,35,17,
  21,5,0,17,77,111,105,115,116,117,
  114,101,0,129,0,4,17,14,5,0,
  17,87,97,116,101,114,0,129,0,35,
  53,21,5,0,17,83,121,115,116,101,
  109,0,66,130,36,25,13,10,0,2,
  26,65,10,51,27,7,7,0,65,12,
  51,37,7,7,0,67,4,36,38,9,
  6,0,2,13,11,129,0,45,39,5,
  5,0,17,37,0 };
  
// this structure defines all the variables of your control interface 
struct {

    // output variable
  int8_t level_1; // =0..100 level position 
  uint8_t led_esp_r; // =0..255 LED Red brightness 
  uint8_t led_esp_g; // =0..255 LED Green brightness 
  uint8_t led_pump_r; // =0..255 LED Red brightness 
  uint8_t led_pump_g; // =0..255 LED Green brightness 
  uint8_t led_gsm_r; // =0..255 LED Red brightness 
  uint8_t led_gsm_g; // =0..255 LED Green brightness 
  int8_t level_2; // =0..100 level position 
  uint8_t led_wet_g; // =0..255 LED Green brightness 
  uint8_t led_dry_r; // =0..255 LED Red brightness 
  char text_1[11];  // string UTF8 end zero 

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)

/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////

SoftwareSerial SIM900(10,11); //rx,tx
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int moisture1_pin = A0;
const int moisture2_pin = A1;
const int valve1_pin = 6;
const int valve2_pin = 5;
const int trigPin = 7;
const int echoPin = 8;

long duration;
double initDistance;
double distance;

boolean soilDry = false;
boolean waterCritical = false;

void setup(){
  pinMode(moisture1_pin, INPUT);
  pinMode(moisture2_pin, INPUT);
  pinMode(valve1_pin, OUTPUT);
  pinMode(valve2_pin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(valve1_pin, HIGH);
  digitalWrite(valve2_pin, HIGH);
  lcd.begin();
  lcd.backlight();
  lcd.print("Welcome To");
  lcd.setCursor(8, 1);
  lcd.print("MAIMS");
  delay(4000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Initializing ESP....");
  RemoteXY_Init ();
  delay(1000);
  lcd.clear();
  lcd.print("ESP8266 OK");
  RemoteXY.led_esp_g = 250;
  delay(2000);
  lcd.clear();
  lcd.print("Initializing GSM....");
  SIM900.begin(115200);
  delay(10000);
  lcd.clear();
  lcd.print("GSM OK");
  RemoteXY.led_gsm_g = 250;
  delay(2000);
  lcd.clear();
}

void loop(){ 
  RemoteXY_Handler ();
  
  getMoisture1();
  getMoisture2();
  getLevel();
  // TODO you loop code
  // use the RemoteXY structure for data transfer

  delay(100);
}

void getMoisture1(){
  if(!waterCritical){
    double mVal = analogRead(moisture1_pin);
    double m_percent = (800 - mVal) / 800 * 100;
    //lcd.clear();
    lcd.setCursor(0,0);
    if(m_percent > 25){
      digitalWrite(valve1_pin, HIGH);
      RemoteXY.led_pump_r = 250;
      RemoteXY.led_pump_g = 0;
      lcd.print("Moisture: " + String(m_percent) + "%");
      RemoteXY.level_2 = m_percent;
      RemoteXY.led_wet_g = 250;
      RemoteXY.led_dry_r = 0;
      dtostrf(m_percent, 0, 0, RemoteXY.text_1);
      if(soilDry){
        send_sms("Your Plant in box 1 Has Been Watered!");
      }
      soilDry = false;
    }
    else{
      digitalWrite(valve1_pin, LOW);
      RemoteXY.led_pump_g = 250;
      RemoteXY.led_pump_r = 0;
      lcd.print("Moisture = DRY!");
      lcd.setCursor(0,1);
      lcd.print("Water Pump: ON");
      RemoteXY.level_2 = m_percent;
      RemoteXY.led_dry_r = 250;
      RemoteXY.led_wet_g = 0;
      dtostrf(m_percent, 0, 0, RemoteXY.text_1);
      if(!soilDry){
        send_sms("Your Plant in box 1 is Thirsty! Pump System ON");
      }
      soilDry = true;
    }
  }
}

void getMoisture2(){
  if(!waterCritical){
    double mVal = analogRead(moisture2_pin);
    double m_percent = (800 - mVal) / 800 * 100;
    //lcd.clear();
    lcd.setCursor(0,0);
    if(m_percent > 25){
      digitalWrite(valve2_pin, HIGH);
      //RemoteXY.led_pump_r = 250;
     // RemoteXY.led_pump_g = 0;
      //lcd.print("Moisture: " + String(m_percent) + "%");
      //RemoteXY.level_2 = m_percent;
      //RemoteXY.led_wet_g = 250;
      //RemoteXY.led_dry_r = 0;
      //dtostrf(m_percent, 0, 0, RemoteXY.text_1);
      if(soilDry){
        send_sms("Your Plant in box 2 Has Been Watered!");
      }
      soilDry = false;
    }
    else{
      digitalWrite(valve2_pin, LOW);
      //RemoteXY.led_pump_g = 250;
      //RemoteXY.led_pump_r = 0;
      //lcd.print("Moisture = DRY!");
      //lcd.setCursor(0,1);
      //lcd.print("Water Pump: ON");
      //RemoteXY.level_2 = m_percent;
      //RemoteXY.led_dry_r = 250;
      //RemoteXY.led_wet_g = 0;
      //dtostrf(m_percent, 0, 0, RemoteXY.text_1);
      if(!soilDry){
        send_sms("Your Plant in box 2 is Thirsty! Pump System ON");
      }
      soilDry = true;
    }
  }
}

void getLevel(){
  if(!soilDry){
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    initDistance = duration * 0.034 / 2;
    distance = 35 - initDistance;
    //lcd.clear();
    lcd.setCursor(0,1);
    double h_percent = (distance / 35) * 100;
    if(h_percent > 20){
      lcd.print("Water lvl: " + String(h_percent) + "%");
      RemoteXY.level_1 = h_percent;
      waterCritical = false;
    }
    else{
      lcd.setCursor(0,0);
      lcd.print("Water Level: ");
      lcd.setCursor(5,1);
      lcd.print("CRITICAL!");
      RemoteXY.level_1 = h_percent;
      
      send_sms("The Water in Your Tank is Critical Below 20%! Please Refill Immediately");
      delay(20000);
      
      if(!waterCritical){
        
      }
      waterCritical = true;
    }
  }
}

void send_sms(String msg){
  SIM900.print("AT+CMGF=1\r"); 
  delay(100);

  //String num="639156781264";
  //String mess="AT+CMGS=\"+"+ num +"\"\r";
  //SIM900.println(mess);
  
  SIM900.println("AT + CMGS = \"+639156781264\""); 
  delay(100);
  
  SIM900.println(msg); 
  delay(100);

  SIM900.println((char)26); 
  delay(100);
  SIM900.println();
  delay(5000); 
}
