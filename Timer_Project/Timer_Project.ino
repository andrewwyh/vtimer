#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <limits.h>
#include <SPI.h>
#include <SdFat.h>

//#define SERIAL_DEBUG  // uncomment for Serial debug
//#define DEBUG_ON //uncomment for memory and SD read times

#define MANUAL_GONG_DURATION 60000 //miliseconds for Manual Gong

/* Define an array of all available courses, as well as the length for each course type */
/* this part includes 20 days

#define ARRAYSIZE 7 //how many courses are defined.

char course_type[ARRAYSIZE][20]=
{   "Between Crs",
    "10-Day",
    "3-Day",
    "Satipatthana",
    "1-Day",
    "20-Day",
    "30-Day",
};

int course_length[ARRAYSIZE] = {1,12,4,10,1,22,32};
*/

#define ARRAYSIZE 5  //how many courses are defined.

char course_type[ARRAYSIZE][20]=
{   "Between Crs",
    "10-Day",
    "3-Day",
    "Satipatthana",
    "1-Day"
};

int course_length[ARRAYSIZE] = {1,12,4,10,1};


/* From here on, it's all constants
 *  
 */
 
LiquidCrystal_I2C lcd(0x27,20,4); // Display  I2C 20 x 
RTC_DS1307 RTC;
DateTime now;

/* The Pins connected to which buttons and relay */

#define relayPin 5 // the relay D1 is connected to this pin D5
#define CS_PIN 4  //this is the pin used for CS from SD card D4
#define SELECT 6 // Button SET MENU', D6
#define UP 7 // Button +, D7
#define DOWN 8 // Button -, D8
#define MANUAL_GONG 9 // manual gong, D9

bool gong;         //whether gong is on or off
bool manual_gong;  //whether manual gong is on or off
bool manual_gong_debounce;  //this variable is to make sure the manual gong doesn't turn off immediately if the user holds the button
bool is_clock_adjusted; //is the clock adjusted? If not, we only update the course type and day
bool manual_parse_sd=1; //this variable triggers parse SD any time (in the next loop()). Set to 1 initially because we want to parse immediately after turning on
unsigned long manual_gong_time; //variable to count the manual gong time
bool sd_card_read; //prevents the SD card from being read twice (or more) at 0s, if reading takes less than a second

int current_day=0;  //this variable marks which course day it is
int current_course=0; //this variable marks which course it is
int menu =0;

int hourupg; // for hour setting
int minupg; // for minute setting

void setup()
{
  /* Sets the input and output pins */
  
  pinMode(relayPin,OUTPUT);
  pinMode(SELECT,INPUT_PULLUP);
  pinMode(UP,INPUT_PULLUP);
  pinMode(DOWN,INPUT_PULLUP);
  pinMode(MANUAL_GONG,INPUT_PULLUP);

  /* Inits the LCD */
  lcd.init();
  lcd.backlight();
  lcd.clear();

  /* Serial debug
   *  
   */
   
  #ifdef SERIAL_DEBUG
  Serial.begin(9600);
  #endif
  
  Wire.begin();
  RTC.begin();

  if (! RTC.isrunning()) {
    
    #ifdef SERIAL_DEBUG
    Serial.println(F("RTC is NOT running!"));
    #endif
    
    lcd.setCursor(0,3);
    lcd.print(F("RTC not runng!"));
    // Set the date and time at compile time
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  // RTC.adjust(DateTime(__DATE__, __TIME__)); //removing "//" to adjust the time

  showWelcome();  //shows Welcome message
 }

void showWelcome(){
  lcd.setCursor(0,0);
  lcd.print(F("Gong Timer for"));
  lcd.setCursor(0,1);
  lcd.print(F("Vipassana Courses"));
  lcd.setCursor(0,2);
  lcd.print(F("Build "));
  lcd.print(__DATE__);  //fills in the date of compilation
  delay(1000);
  lcd.clear();
}

#ifdef DEBUG_ON
/* This function reports how much free RAM is available on the Arduino. We need to check for memory leaks */

int getFreeRam()
{
  extern int __heap_start, *__brkval; 
  int v;

  v = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  return v;
}
#endif

void loop() { 

/* checks and prints Free RAM with every loop iteration */

#ifdef DEBUG_ON
lcd.setCursor(15,1);
lcd.print(getFreeRam(),DEC);
#endif

/* Updates the time with every loop */
now = RTC.now();

/* Prints the "GONG ON" or "GONG OFF" on the LCD */
if (gong==0){
  lcd.setCursor(12,3);
  lcd.print(F("GONG OFF"));
  }
if (gong==1&&manual_gong==0){
  lcd.setCursor(12,3);
  lcd.print(F("GONG ON "));
  }

/* If Manual Gong is active, do a countdown to end of Gong. Also allows the user to cancel the gong by pressing the button again */

if (manual_gong==1){
  
  lcd.setCursor(0,3);
  lcd.print(F("Manual Gong: "));
  lcd.print((MANUAL_GONG_DURATION-(millis()-manual_gong_time))/1000);

  /* This statement checks if button has been released. Otherwise, if the user holds the manual button, to prevent manual gong to be immediately stopped */
  
  if (digitalRead(MANUAL_GONG)==HIGH)
    manual_gong_debounce=0;

  /* If countdown time has passed, or if the user presses the button again, manual gong will be shut off */
    if (((millis()-manual_gong_time)>MANUAL_GONG_DURATION)||(digitalRead(MANUAL_GONG)==LOW&&manual_gong_debounce==0)){
        manual_gong=0;
        switchgong(0);
        lcd.setCursor(0,3);
        lcd.print(F("                    "));
        manual_parse_sd=1; //read SD card again after manual gong shutoff
        delay(500);
  }
}

/* if Manual Gong button is pressed */

if(digitalRead(MANUAL_GONG)==LOW&&manual_gong_debounce==0)
  {
   /* clear the last row for Manual Gong countdown display*/
   lcd.setCursor(0,3);
   lcd.print(F("                    "));
   
   manual_gong_debounce=1;
   manualgong();
  }

/* check if you press the SET button and increase the menu index */

if(digitalRead(SELECT)==LOW)
  {
   menu=menu+1;
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
    manual_parse_sd=1;
    }
    delay(100);

/* Reads from the SD card every 0 seconds, i.e. every minute. Prevents SD card from reading second time if already read.
 *  Also, read it it manual_parse_sd is triggered
*/

if ((now.second()==0&&sd_card_read==0)||manual_parse_sd==1) {
  parse_sd(); //read from SD card
  manual_parse_sd=0; // clears after we parse it
  sd_card_read=1;
}

if (now.second()==50){
  sd_card_read=0; // resets this variable every 50s.
  }
}

/* This function is called when Manual Gong is pressed */
void manualgong(){

  if (gong==1) {  // if gong is already on, do nothing
     return;
  }

  manual_gong_time=millis();
  manual_gong=1;
  switchgong(1);
}

/* By default, this function will be called to display the course, day, and time */
void DisplayDateTime ()
{
// We show the current course, day and time

  lcd.setCursor(0, 0);
  lcd.print(F("Course: "));
  current_course=now.year()-2000;
  lcd.print(course_type[current_course]);
 
  current_day=getcurrentday(now);
    
  lcd.setCursor(0,1);
    if (course_length[current_course]==1)
      lcd.print("              ");

    else {
      lcd.print("Today is day ");
      lcd.print(current_day);
    }
  
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
}

/* Sets the Course Type */
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

/* Sets the Course Day */
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
    if (current_day>course_length[current_course]-1)
      current_day=0;
  lcd.setCursor(0,1);
  lcd.print(current_day,DEC);
  delay(200);
}

/* Sets Hour */
void DisplaySetHour()
{
// time setting
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Set Time"));
  lcd.setCursor(0,1);
  lcd.print(F("Set hour:"));
  lcd.setCursor(0,2);
  lcd.print(hourupg,DEC);
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

  delay(200);
}

/* Sets the minute */

void DisplaySetMinute()
{
// Setting the minutes
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Set Time"));
  lcd.setCursor(0,1);
  lcd.print(F("Set Minutes:"));
  lcd.setCursor(0,2);
  lcd.print(minupg,DEC);
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
  delay(200);
}

/* Stores the courses into the RTC chip */
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

/* Only adjusts the clock if the hour or minute has moved.
 *  This prevents from the clock from being updated every time, i.e. it will start from zero seconds and the time wil drift.
 */
  if (is_clock_adjusted ==1) {
    RTC.adjust(DateTime(current_course,tmpmonth,tmpday,hourupg,minupg,0));
    is_clock_adjusted=0;
  }
  else {
  RTC.adjustbrief(DateTime(current_course,tmpmonth,tmpday,hourupg,minupg,0));
  }

/* Reads from RTC after writing to it */

  now=RTC.now();
  delay(200);
}

/* this function is called to read the calendar to determine which course day it is.
 *  1st Jan is Day 0. Jan 31st is Day 30. Feb 1st is Day 31st, and so on.
 */
int getcurrentday(DateTime now){
  
  int current_day_tmp;

  /*This function is good until 45 days. For longer courses (60 day), it will leak to March and the logic doesn't work. */
  
  current_day_tmp = (now.month()-1)*31+now.day()-1;  

  /* If the current day is more than the course length, we will jump to "Between Course Period" */
  
  if (current_day_tmp>course_length[current_course]-1){
    current_course=0; // set to Between course period 
    current_day=0;
    StoreAgg(); 
    return 0; // set to day 0
      }
   else return current_day_tmp;
}

/* This function is called every minute, when the unit is switched on, when manual gong has stopped.
 *   It reads from the SD card, and then sets the relay on or off accordingly
 */
void parse_sd() {

SdFat SD;
File file;

int timer_data[5]; // only 5 variables. Course type, Course day, Hour, minute and on/off
int next_timer_hour; // Next timer variable
int next_timer_minute; //Next time variable
bool next_timer_available; //Shows whether Next timer already set, so that it will ignore all future values

int i=0; 
int read_value; //temporary read value from SD card is stored here

#ifdef DEBUG_ON
unsigned long int time1=millis(); //this variable counts how much time is needed to read the whole file
#endif

  // Initialize the SD card
  if (!SD.begin(CS_PIN)){
  lcd.setCursor(0,3);
  lcd.print(F("SD init failed"));
  return;
  }
  
  // Open the file.

  file = SD.open("timer.csv", FILE_READ);

  if (!file){
    lcd.setCursor(0,3);
    lcd.print(F("File open fail"));
    return;
  }
  
  // Rewind the file for read.
  file.seek(0);

  int n;      // Length of returned field with delimiter.

#ifdef SERIAL_DEBUG
  Serial.print(F("Current course is "));
  Serial.println(current_course);
  Serial.print(F("Current day is "));
  Serial.println(current_day);
  Serial.print(F("Hour is "));
  Serial.println(now.hour());
  Serial.print(F("Minute is "));
  Serial.println(now.minute());
#endif

  // Read the file and print fields.
 
  lcd.setCursor(0,3);
  lcd.print(F("Read SD     "));
  
  next_timer_available=0; //Reset Next Timer available before reading a fresh file
  
  while (file.available()) {

    n = csvReadInt16(&file, &read_value, ','); //reads one variable until we hit a comma

    //done if Error or at EOF.
    if (n == 0) break;

       /*44 is the int value for a comma, and only read timer_data[1] to timer_data[4] */
      if (n == 44&&i<5){  
            timer_data[i] = read_value;
      }

      i++;

      int temp_next_timer_diff;

      /* this is when we hit newline. Sometimes it's -3, sometimes it's 10
       *  Process the entries by comparing with the current course, day, and time.
       */
      if (n == -3||n==10){   
                 
      if (timer_data[0]-current_course==0){
        #ifdef SERIAL_DEBUG
        Serial.println(F("Match Course"));
        #endif

        if (timer_data[1]-current_day==0){
          #ifdef SERIAL_DEBUG
          Serial.println(F("Match Day"));
          #endif
          
            /* this code to determine next schedule */
            
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

          if (timer_data[2]-now.hour()==0){
           #ifdef SERIAL_DEBUG
           Serial.println(F("Match Hour"));
           #endif
           
            if (timer_data[3]-now.minute()==0){
              #ifdef SERIAL_DEBUG
              Serial.println(F("Match Minute"));
              #endif
              
              if (timer_data[4]==1)
                {
                  if (manual_gong==0) { //switch gong on only if Manual Gong is not already on
                    
                  #ifdef SERIAL_DEBUG
                  Serial.println(F("Gong On"));
                  #endif 
                  
                  switchgong(1);
                  }
                  
                }
              if (timer_data[4]==0)
                {
                  if (manual_gong==0){//switch gong off only if Manual Gong is not active
                  #ifdef SERIAL_DEBUG
                  Serial.println(F("Gong OFF")); 
                  #endif
                  
                  switchgong(0);
                  }
                  
                } //on/off
                
            }//minute
          }//hour
        }//Current Day
      }//Current course
      
      i=0; //reset i to zero for new line
      
    } //new line

  } // While file is still available (break at EOF)

lcd.setCursor(0,3);
lcd.print(F("       "));

#ifdef DEBUG_ON
  /* determine how much time needed to read */
  unsigned long int time2=millis();
  unsigned long int time_taken=time2-time1;
  lcd.setCursor(0,3);
  lcd.print(time_taken);
  lcd.print(F(" ms   "));
#endif

  #ifdef SERIAL_DEBUG
  //Serial.print(F("Time taken is "));
  //Serial.println(time_taken);
    #endif

/* print next schedule */

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
    
 file.close();
  
}

void switchgong(bool i){
  if (i==1){
    if (gong==1) //if gong is already on, ignore
      return;

    if (gong==0){
      digitalWrite(relayPin,HIGH);
      gong=1;
      return;
    }
  }

  if (i==0){
    if (gong==0) //if gong is already off, ignore
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
