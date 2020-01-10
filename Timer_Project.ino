//************libraries**************//
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <limits.h>
#include <SPI.h>

// next two lines for SdFat
#include <SdFat.h>

#define CS_PIN 4
unsigned char relayPin = 5;
bool gong;
bool manual_gong;
bool manual_gong_debounce;
bool is_clock_adjusted;
unsigned long manual_gong_time;
#define MANUAL_GONG_DURATION 10000 //miliseconds for Manual Gong

LiquidCrystal_I2C lcd(0x27,20,4); // Display  I2C 20 x 
RTC_DS1307 RTC;

/* Define an array of all available courses, as well as the length for each course type */
/*
char course_type[ARRAYSIZE][20]=
{   "Between Crs",
    "10-Day",
    "3-Day",
    "Satipatthana",
    "20-Day",
    "30-Day",
};


int course_length[ARRAYSIZE] = {1,12,4,10,22,32};
*/

#define ARRAYSIZE 4  //how many courses are defined.

char course_type[ARRAYSIZE][20]=
{   "Between Crs",
    "10-Day",
    "3-Day",
    "Satipatthana",
};

int course_length[ARRAYSIZE] = {1,12,4,10};

int current_day=0;
int current_course=0;

//************Button*****************//
int SELECT=6; // Button SET MENU'
int UP=7; // Button +
int DOWN=8; // Button -
int MANUAL_GONG=9; // manual gong

int hourupg;
int minupg;

int menu =0;

int timer_data[5];
int next_timer_hour;
int next_timer_minute;
bool next_timer_available;

DateTime now;

void setup()
{

  pinMode(relayPin,OUTPUT);
  pinMode(SELECT,INPUT_PULLUP);
  pinMode(UP,INPUT_PULLUP);
  pinMode(DOWN,INPUT_PULLUP);
  pinMode(MANUAL_GONG,INPUT_PULLUP);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();

  Serial.begin(9600);
  Wire.begin();
  RTC.begin();

  if (! RTC.isrunning()) {
    //Serial.println(F("RTC is NOT running!"));
  lcd.setCursor(0,3);
  lcd.print(F("RTC not runng!"));
    // Set the date and time at compile time
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
    // RTC.adjust(DateTime(__DATE__, __TIME__)); //removing "//" to adjust the time
    // The default display shows the date and time

  showWelcome();
  now=RTC.now();  
  parse_sd(now);
 }

void showWelcome(){
  lcd.setCursor(0,0);
  lcd.print(F("Gong Timer for"));
  lcd.setCursor(0,1);
  lcd.print(F("Vipassana Courses"));
  lcd.setCursor(0,2);
  lcd.print(F("Build "));
  lcd.print(__DATE__);
  delay(1000);
  lcd.clear();
}

int getFreeRam()
{
  extern int __heap_start, *__brkval; 
  int v;

  v = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  return v;
}

void loop()
{ 

lcd.setCursor(15,1);
lcd.print(getFreeRam(),DEC);

now = RTC.now();

if (now.second()==0){
  parse_sd(now); //read from SD card
}

if (gong==0){
  lcd.setCursor(12,3);
  lcd.print(F("GONG OFF"));
  }
if (gong==1&&manual_gong==0){
  lcd.setCursor(12,3);
  lcd.print(F("GONG ON "));
  }

if (manual_gong==1){
  lcd.setCursor(0,3);
  lcd.print(F("Manual Gong: "));
  lcd.print((MANUAL_GONG_DURATION-(millis()-manual_gong_time))/1000);
  lcd.print(F("       "));

  if (digitalRead(MANUAL_GONG)==HIGH)
    manual_gong_debounce=0;

  if (((millis()-manual_gong_time)>MANUAL_GONG_DURATION)||(digitalRead(MANUAL_GONG)==LOW&&manual_gong_debounce==0)){
        manual_gong=0;
        switchgong(0);
        lcd.setCursor(0,3);
        lcd.print(F("                  "));
        parse_sd(now);
        delay(500);
  }
}

// check if you press the SET button and increase the menu index
  if(digitalRead(SELECT)==LOW)
  {
   menu=menu+1;
  }

    if(digitalRead(MANUAL_GONG)==LOW&&manual_gong_debounce==0)
  {
   manual_gong_debounce=1;
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
    parse_sd(now);
    }
    delay(100);
}

void select(){
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
// We show the current course, day and time

  lcd.setCursor(0, 0);
  lcd.print(F("Course: "));
  current_course=now.year()-2000;
  lcd.print(course_type[current_course]);
 
  current_day=getcurrentday(now);
    
  lcd.setCursor(0,1);
  lcd.print("Today is day ");
  lcd.print(current_day);
  
  lcd.setCursor(0, 2);
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
  */
  
  
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
  if(digitalRead(UP)==LOW)
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
   if(digitalRead(DOWN)==LOW)
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
  lcd.print(F("Set hour:"));
  lcd.setCursor(0,1);
  lcd.print(hourupg,DEC);
  delay(200);
}

void DisplaySetMinute()
{
// Setting the minutes
  lcd.clear();
  if(digitalRead(UP)==LOW)
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
   if(digitalRead(DOWN)==LOW)
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
  lcd.print(F("Set Minutes:"));
  lcd.setCursor(0,1);
  lcd.print(minupg,DEC);
  delay(200);
}
  
void DisplaySetCourseType()
{
// setting the course type
   
  lcd.clear();
  if(digitalRead(UP)==LOW)
  {    
    current_course=current_course+1;

    if (current_course>ARRAYSIZE-1)
      current_course=0;
  }
   if(digitalRead(DOWN)==LOW)
  {
    current_course=current_course-1;
    
    if (current_course<0)
      current_course=ARRAYSIZE-1;
  }
  
  lcd.setCursor(0,0);
  lcd.print(F("Set Course Type"));
  lcd.setCursor(0,1);
  lcd.print(course_type[current_course]);
  delay(200);
}

void DisplaySetDay()
{
// Setting the day
  lcd.clear();
  if(digitalRead(UP)==LOW)
  {
    current_day=current_day+1;
    if (current_day>course_length[current_course]-1)
    {
      current_day=0;
    }

  }
   if(digitalRead(DOWN)==LOW)
  {
    current_day=current_day-1;
    if (current_day<0)
    {
      current_day=course_length[current_course]-1;
    }
    
  }
  lcd.setCursor(0,0);
  lcd.print(F("Set Day:"));
  lcd.setCursor(0,1);
  lcd.print(current_day,DEC);
  delay(200);
}

void StoreAgg()
{
// Variable saving
  int tmpmonth=1;
  int tmpday=1;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("SAVING IN"));
  lcd.setCursor(0,1);
  lcd.print(F("PROGRESS"));

  if (current_day>30) {
    tmpmonth++;
    tmpday=current_day%30;  //OK until 45 days course. For 60 days course, this logic doesn't work
  }
  else {
  tmpday = current_day+1;
  }
  
  if (is_clock_adjusted ==1) {
    RTC.adjust(DateTime(current_course,tmpmonth,tmpday,hourupg,minupg,0));
    is_clock_adjusted=0;
  }
  else {
  RTC.adjustbrief(DateTime(current_course,tmpmonth,tmpday,hourupg,minupg,0));
  }

  now=RTC.now();
  
  delay(200);
}

void parse_sd(DateTime now) {

SdFat SD;
File file;

  int i=0;
  int read_value;
  unsigned long int time1=millis();
  //Serial.println(F("Timer Started"));

  // Initialize the SD card
  //if (!SD.begin(CS_PIN)) Serial.print(F("begin failed"));
  if (!SD.begin(CS_PIN)){
  lcd.setCursor(0,3);
  lcd.print(F("SD init failed"));
  return;
  }
  
  // Open the file.

  file = SD.open("timer.csv", FILE_READ);
  //if (!file) Serial.print(F("open failed"));

  if (!file){
    lcd.setCursor(0,3);
    lcd.print(F("File open fail"));
    return;
  }
  
  // Rewind the file for read.
  file.seek(0);

  int n;      // Length of returned field with delimiter.

  Serial.print(F("Current course is "));
  Serial.println(current_course);
  Serial.print(F("Current day is "));
  Serial.println(current_day);
  Serial.print(F("Hour is "));
  Serial.println(now.hour());
  Serial.print(F("Minute is "));
  Serial.println(now.minute());

  // Read the file and print fields.
 

  lcd.setCursor(0,3);
  lcd.print(F("Read SD     "));
  
  while (file.available()) {

       
    n = csvReadInt16(&file, &read_value, ',');

    //done if Error or at EOF.
    if (n == 0) break;
      
      if (n == 44){   //44 is the int value for a comma
            timer_data[i] = read_value;
      }

      i++;

      int temp_next_timer_diff;
      if (n == -3||n==10){   //-3 is the int value for \n, sometimes it's 10
        
           temp_next_timer_diff=(timer_data[2]*60+timer_data[3])-(now.hour()*60+now.minute());
           
           if (temp_next_timer_diff>0){

            if (next_timer_available==0) {
            next_timer_available=1;
            //next_timer=timer_data[2]*100+timer_data[3];
            next_timer_hour=timer_data[2];
            next_timer_minute=timer_data[3];
            }

           }
           else next_timer_available=0;
                 
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
                  if (manual_gong==0) { //switch gong on only if Manual Gong is not already on
                  Serial.println(F("Gong On"));
                  switchgong(1);
                  }
                  
                }
              if (timer_data[4]==0)
                {
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
  
  unsigned long int time2=millis();
  unsigned long int time_taken=time2-time1;
  //Serial.print(F("Time taken is "));
  //Serial.println(time_taken);
  lcd.setCursor(10,2);

        if (next_timer_available){
          lcd.print(F("Next "));
            if (next_timer_hour<=9){
            lcd.print("0");          
            }
           lcd.print(next_timer_hour);
           lcd.print(F(":"));
             if (next_timer_minute<=9){
            lcd.print("0");          
            }
            lcd.print(next_timer_minute);
            
      }
      else 
        lcd.print(F("No next tmr"));
    
  lcd.setCursor(0,3);
  lcd.print(time_taken);
  lcd.print(F(" ms   "));
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
