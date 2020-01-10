//************libraries**************//
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <limits.h>
#include <SPI.h>

// next line for SD.h
//#include <SD.h>

// next two lines for SdFat
#include <SdFat.h>
SdFat SD;

#define CS_PIN 4
unsigned char relayPin = 5;
bool gong;
bool manual_gong;
bool is_clock_adjusted;
unsigned int manual_gong_time;
#define MANUAL_GONG_DURATION 10000 //miliseconds for Manual Gong

File file;

//************************************//
LiquidCrystal_I2C lcd(0x27,20,4); // Display  I2C 20 x 
RTC_DS1307 RTC;

#define ARRAYSIZE 5  //how many courses are defined. Currently set to 5

/* Define an array of all available courses, as well as the length for each course type */

char course_type[ARRAYSIZE][20]=
{   "Between Course",
    "10-Day",
    "3-Day",
    "1-Day",
    "Satipatthana"
};

int course_length[ARRAYSIZE] = {1,12,5,1,10};

int current_day=0;
int current_course=0;

//************Button*****************//
int P1=6; // Button SET MENU'
int P2=7; // Button +
int P3=8; // Button -
int P4=2; // manual Gong

/*
// Interrupt 0 is hardware pin 4 (digital pin 2)
int btnselect = 0;

// Interrupt 1 is hardware pin 5 (digital pin 3)
int btnmanualgong = 1;
*/

//************Variables**************//
int hourupg;
int minupg;

int menu =0;

int timer_data[5];

void setup()
{
/*
  attachInterrupt(btnselect , select, RISING);
  attachInterrupt(btnmanualgong, manualgong, RISING);
*/

  pinMode(relayPin,OUTPUT);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(P1,INPUT); //replaced by interrupt 0
  pinMode(P2,INPUT);
  pinMode(P3,INPUT);
  pinMode(P4,INPUT);

  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
   
  //Serial.print(F("Program started\n"));

  // Initialize the SD card
  if (!SD.begin(CS_PIN)) Serial.print(F("begin failed"));
   
  if (! RTC.isrunning()) {
    Serial.println(F("RTC is NOT running!"));
    // Set the date and time at compile time
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  // RTC.adjust(DateTime(__DATE__, __TIME__)); //removing "//" to adjust the time
    // The default display shows the date and time
  int menu=0;
 }
 
void loop()
{ 

if (gong==0){
  lcd.setCursor(12,3);
  lcd.print("GONG OFF");
  }
if (gong==1){
  lcd.setCursor(12,3);
  lcd.print("GONG ON ");
  }

if (manual_gong==1){
  lcd.setCursor(0,3);
  lcd.print("Manual Gong: ");
  lcd.print((MANUAL_GONG_DURATION-(millis()-manual_gong_time))/1000);
  lcd.print("       ");
  
  if ((millis()-manual_gong_time)>MANUAL_GONG_DURATION){
        manual_gong=0;
        switchgong(0);
        lcd.setCursor(0,3);
        lcd.print("             ");
  }
}

// check if you press the SET button and increase the menu index
  if(digitalRead(P1)) // replaced by interrupt 0
  {
   menu=menu+1;
  }

    if(digitalRead(P4)) // replaced by interrupt 0
  {
   manualgong();
  }

// in which subroutine should we go?
  if (menu==0)
    {
     DisplayDateTime(); // void DisplayDateTime

    }
  if (menu==1)
    {
    DisplaySetCourseType();
    
    }
  if (menu==2)
    {
    DisplaySetDay();
    
    }
  if (menu==3)
    {
    DisplaySetHour();
    }
  if (menu==4)
    {
    DisplaySetMinute();
    }
  if (menu==5)
    {
    StoreAgg(); 
    delay(500);
    menu=0;
    switchgong(0);
    }
    delay(100);
}

// Interrupt function
void select(){
  Serial.print(F("Select Pressed\n"));
     menu=menu+1;
}

void manualgong(){

  if (gong==1) {  // if gong is already on, do nothing
     return;
  }

  manual_gong_time=millis();
  manual_gong=1;
  switchgong(1);
 
}

void DisplayDateTime ()
{
// We show the current date and time
  
  DateTime now = RTC.now();

  lcd.setCursor(0, 2);
  //lcd.print("Hour:");
  if (now.hour()<=9)
  {
    lcd.print("0");
  }
  lcd.print(now.hour(), DEC);
  hourupg=now.hour();
  lcd.print(":");
  if (now.minute()<=9)
  {
    lcd.print("0");
  }
  lcd.print(now.minute(), DEC);
  minupg=now.minute();
  lcd.print(":");
  if (now.second()<=9)
  {
    lcd.print("0");
  }
  lcd.print(now.second(), DEC);

/*
  lcd.setCursor(0, 0);
  
  lcd.print("Date: ");
*/
  lcd.print(" ");
  if (now.day()<=9)
  {
    lcd.print("0");
  }
  lcd.print(now.day(), DEC);
  lcd.print("/");
  if (now.month()<=9)
  {
    lcd.print("0");
  }
  lcd.print(now.month(), DEC);
  lcd.print("/");
  lcd.print(now.year(), DEC);
  
  
  lcd.setCursor(0, 0);
  lcd.print("Course: ");
  current_course=now.year()-2000;
  lcd.print(course_type[current_course]);
  
  current_day=getcurrentday(now);
    
  lcd.setCursor(0,1);
  lcd.print("Today is day ");
  lcd.print(current_day);
  
  parse_sd(now);
  
}

int getcurrentday(DateTime now){
  
  int current_day_tmp;
  current_day_tmp = (now.month()-1)*31+now.day()-1;

//always Day 1 for 1-day courses, TBD: need to make a logic for this
  
  if (current_day_tmp>course_length[current_course]-1){
    current_course=0; // set to Between course period once 
    current_day=0;
    StoreAgg(); 
    return 0; // set to day 0
      }
   else return current_day_tmp;
  
}

void DisplaySetHour()
{
// time setting
  lcd.clear();
  if(digitalRead(P2)==HIGH)
  {
    is_clock_adjusted = 1;
    if(hourupg==23)
    {
      hourupg=0;
    }
    else
    {
      hourupg=hourupg+1;
    }
  }
   if(digitalRead(P3)==HIGH)
  {
    is_clock_adjusted = 1;
    if(hourupg==0)
    {
      hourupg=23;
    }
    else
    {
      hourupg=hourupg-1;
    }
  }
  lcd.setCursor(0,0);
  lcd.print("Set time:");
  lcd.setCursor(0,1);
  lcd.print(hourupg,DEC);
  delay(200);
}

void DisplaySetMinute()
{
// Setting the minutes
  lcd.clear();
  if(digitalRead(P2)==HIGH)
  {
    is_clock_adjusted = 1;
    
    if (minupg==59)
    {
      minupg=0;
    }
    else
    {
      minupg=minupg+1;
    }
  }
   if(digitalRead(P3)==HIGH)
  {
     is_clock_adjusted = 1;
    if (minupg==0)
    {
      minupg=59;
    }
    else
    {
      minupg=minupg-1;
    }
  }
  lcd.setCursor(0,0);
  lcd.print("Set Minutes:");
  lcd.setCursor(0,1);
  lcd.print(minupg,DEC);
  delay(200);
}
  
void DisplaySetCourseType()
{
// setting the course type
  //current_course=0; //not sure why, if we didn't set this, the variable will be 19
  
   Serial.print(F("Current course is "));
   Serial.println(current_course);
   
  lcd.clear();
  if(digitalRead(P2)==HIGH)
  {    
    current_course=current_course+1;

    if (current_course>ARRAYSIZE-1)
      current_course=0;
  }
   if(digitalRead(P3)==HIGH)
  {
    current_course=current_course-1;
    
    if (current_course<0)
      current_course=ARRAYSIZE-1;
  }
  
  lcd.setCursor(0,0);
  lcd.print("Set Course Type");
  lcd.setCursor(0,1);
  lcd.print(course_type[current_course]);
  delay(200);
}

void DisplaySetDay()
{
// Setting the day
  lcd.clear();
  if(digitalRead(P2)==HIGH)
  {
    current_day=current_day+1;
    if (current_day>course_length[current_course]-1)
    {
      current_day=0;
    }

  }
   if(digitalRead(P3)==HIGH)
  {
    current_day=current_day-1;
    if (current_day<0)
    {
      current_day=course_length[current_course]-1;
    }
    
  }
  lcd.setCursor(0,0);
  lcd.print("Set Day:");
  lcd.setCursor(0,1);
  lcd.print(current_day,DEC);
  delay(200);
}

void StoreAgg()
{
// Variable saving
  int tmpmonth=1;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("SAVING IN");
  lcd.setCursor(0,1);
  lcd.print("PROGRESS");

  if (current_day>30) {
    tmpmonth++;
    current_day=current_day-30; //double-check this logic. January has 31 days
  }
  else {
  current_day = current_day+1;
  }
  
  if (is_clock_adjusted ==1) {
    RTC.adjust(DateTime(current_course,tmpmonth,current_day,hourupg,minupg,0));
    is_clock_adjusted=0;
  }
  else {
  RTC.adjustbrief(DateTime(current_course,tmpmonth,current_day,hourupg,minupg,0));
  }
  delay(200);
}

void parse_sd(DateTime now) {

  // Open the file.

  file = SD.open("timer.txt", FILE_READ);
  if (!file) Serial.print(F("open failed"));
  
  // Rewind the file for read.
  file.seek(0);

  int n;      // Length of returned field with delimiter.

  // Read the file and print fields.
 
  int i=0;
  int read_value;
  int time1=millis();
  
  Serial.println(F("Timer Started"));

  while (file.available()) {

       
    n = csvReadInt16(&file, &read_value, ',');

    //done if Error or at EOF.
    if (n == 0) break;
      //Serial.print(F("n is "));
      //Serial.println(n);
      
      if (n == 44){   //44 is the int value for a comma
            timer_data[i] = read_value;
      }

      i++;
      
      if (n == -3||n==10){   //-3 is the int value for \n, sometimes it's 10

      /*
      Serial.print(F("timer_data[0] is "));
      Serial.println(timer_data[0]);
      Serial.print(F("timer_data[1] is "));
      Serial.println(timer_data[1]);
      Serial.print(F("timer_data[2] is "));
      Serial.println(timer_data[2]);
      Serial.print(F("timer_data[3] is "));
      Serial.println(timer_data[3]);
      Serial.print(F("timer_data[4] is "));
      Serial.println(timer_data[4]);
      Serial.println(F("--------")); 
      */
      /*
      
      Serial.print(F("Course type is  "));
      Serial.println(current_course);
      Serial.print(F("Course day is  "));
      Serial.println(current_day);
      Serial.print(F("Hour is "));
      Serial.println(now.hour());
      Serial.print(F("Minute is "));
      Serial.println(now.minute());
      */
      
      if (timer_data[0]-current_course==0){
        //Serial.println(F("Match Course"));

        if (timer_data[1]-current_day==0){
          //Serial.println(F("Match Day"));

          if (timer_data[2]-now.hour()==0){
           // Serial.println(F("Match Hour"));
            if (timer_data[3]-now.minute()==0){
              //Serial.println(F("Match Minute"));
              if (timer_data[4]==1)
                {
                  Serial.print(F("Course "));
                  Serial.print(current_course);
                  Serial.print(F(",Day "));
                  Serial.print(current_day);
                  Serial.print(F(",Hour "));
                  Serial.print(timer_data[2]);
                  Serial.print(F(",Minute "));
                  Serial.println(timer_data[3]);
                  if (manual_gong==0) { //switch gong on only if Manual Gong is not already on
                  Serial.println(F("Gong On"));
                  switchgong(1);
                  }
                  
                }
              if (timer_data[4]==0)
                {
                  Serial.print(F("Course "));
                  Serial.println(current_course);
                  Serial.print(F("Day "));
                  Serial.println(current_day);
                  Serial.print(F("Hour "));
                  Serial.println(timer_data[2]);
                  Serial.print(F("Minute "));
                  Serial.println(timer_data[3]);
                  if (manual_gong==0){
                  Serial.println(F("Gong OFF")); //switch gong off only if Manul Gong is not active
                  switchgong(0);
                  }
                  
                }
                
            }
          }
        }
      }
      
      

      
      i=0; //reset i to zero for new line
      
    }

  }
  
  int time2=millis();
  int time_taken=time2-time1;
  Serial.print(F("Time taken is "));
  Serial.println(time_taken);
 file.close();
  
}

void switchgong(bool i){
  if (i==1){
    if (gong==1)
      return;

    if (gong==0){
      digitalWrite(relayPin,HIGH);
      gong=1;
      return;
    }
  }

  if (i==0){
    if (gong==0)
      return;

    if (gong==1){
      digitalWrite(relayPin,LOW);
      gong=0;
      return;
    }
  }
}

/*
 * Read a file one field at a time.
 *
 * file - File to read.
 *
 * str - Character array for the field.
 *
 * size - Size of str array.
 *
 * delim - csv delimiter.
 *
 * return - negative value for failure.
 *          delimiter, '\n' or zero(EOF) for success.           
 */
 
int csvReadText(File* file, char* str, size_t size, char delim) {
  char ch;
  int rtn;
  size_t n = 0;
  while (true) {
    // check for EOF
    if (!file->available()) {
      rtn = 0;
      break;
    }
    if (file->read(&ch, 1) != 1) {
      // read error
      rtn = -1;
      break;
    }
    // Delete CR.
    if (ch == '\r') {
      continue;
    }
    if (ch == delim || ch == '\n') {
      rtn = ch;
      break;
    }
    if ((n + 1) >= size) {
      // string too long
      rtn = -2;
      n--;
      break;
    }
    str[n++] = ch;
  }
  str[n] = '\0';
  return rtn;
}

int csvReadInt16(File* file, int16_t* num, char delim) {
  int32_t tmp;
  int rtn = csvReadInt32(file, &tmp, delim);
  if (rtn < 0) return rtn;
  if (tmp < INT_MIN || tmp > INT_MAX) return -5;
  *num = tmp;
  return rtn;
}

int csvReadInt32(File* file, int32_t* num, char delim) {
  char buf[20];
  char* ptr;
  int rtn = csvReadText(file, buf, sizeof(buf), delim);
  if (rtn < 0) return rtn;
  *num = strtol(buf, &ptr, 10);
  if (buf == ptr) return -3;
  while(isspace(*ptr)) ptr++;
  return *ptr == 0 ? rtn : -4;
}
