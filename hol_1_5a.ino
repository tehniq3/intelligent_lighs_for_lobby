/* 
intelligent light for lobby - original by Nicu FLORICA (niq_ro)
v.1.0 - initial
v.1.1 - serial monitoring for time to off
v.1.2 - changed colours (8 colors) by button
v.1.3 - stored color in EEPROM
v.1.4 - changed pins as an exiting project https://nicuflorica.blogspot.com/2022/09/sistem-automat-pentru-control-deplasare.html
        + used IR sensors + switch from an encoder
v.1.4.a - added i2c LCD1602 for basic info
v.1.4.b - added symbols for direcions
v.1.5 - added encoder adjustment for basic functions + store main values in EEPROM
v.1.5.a - added change for number of LEDs in strip
*/

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Encoder.h> //  http://www.pjrc.com/teensy/td_libs_Encoder.html
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define adresa  50  // adress for store the
byte zero = 0;  // variable for control the initial read/write the eeprom memory

#define PIN       9  // Which pin on the Arduino is connected to the NeoPixels? 
                      // On Trinket or Gemma, suggest changing this to 1
                      
#define NUMPIXELS 255 // How many NeoPixels are attached to the Arduino?

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define senzor1 6
#define senzor2 5
#define encodersw 4
#define CLK 2
#define DT 3

Encoder knob(DT, CLK);  // Best Performance: both pins have interrupt capability

byte state = 0;  // actual state of lights:
/*
  0 - all lights off
  1 - all lights on
121 - lights on from 1 to 2
120 - lights off from 1 to 2
211 - lights on from 2 to 1
210 - lights off from 2 to 2
*/
byte state0 = 7; // previous state of lights

unsigned long tptrigger = 0;  // store time to last sensor is triggered
unsigned long tpasteptate = 10000;  // time to go to off with animation

// "off" light (0 = off, 255 = max)
byte br0 = 5;  // brightness
byte er = 0;  // red 
byte ge = 1;  // green
byte be = 0;  // blue

byte culoare = 7;   // 1 - red, 2 - green, 4 - blue, 7 - white 

// "on" light (0 = off, 255 = max)
byte br = 25;  // brightness

long pauzamica = 15;

int tptooff = 0;
int tptooff0 = 0;
int tptooff2 = 0;
int tptooff20 = 0;

byte romana = 1; // 1 - romana, 0 - english
byte adresacorecta = 22;

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x3F (or 0x27) for a 16 chars and 2 line display

// https://www.arduino.cc/en/Reference/LiquidCrystalCreateChar
byte inainte[8] = {
  B10000,
  B11000,
  B11100,
  B11111,
  B11100,
  B11000,
  B10000,
};

byte inapoi[8] = {
  B00001,
  B00011,
  B00111,
  B01111,
  B00111,
  B00011,
  B00001,
};

byte shtop[8] = {
  B00000,
  B11011,
  B11011,
  B11011,
  B11011,
  B00000,
  B00000,
};

// These variables are for the push button routine
int buttonstate = 0; //flag to see if the button has been pressed, used internal on the subroutine only
int pushlengthset = 3000; // value for a long push in mS
int pushlength = pushlengthset; // set default pushlength
int pushstart = 0;// sets default push value for the button going low
int pushstop = 0;// sets the default value for when the button goes back high
int durata1 = 0;

int knobval; // value for the rotation of the knob
boolean buttonflag = false; // default value for the button flag

//the variables provide the holding values for the set variables
int setlimbatemp; 
int setbrmintemp;
int setbrmaxtemp;
int setpausemintemp;
int setpausemaxtemp;
int setledstemp;
int ledsmax = 5;

void setup() {
  Serial.begin(9600);
  Serial.println("-");
  Serial.println("Inteligent lights for lobby v.1.4a");
  Serial.println("original software wrote by Nicu FLORICA (niq_ro)");
  Serial.println("-");
pinMode(senzor1, INPUT);
pinMode(senzor2, INPUT);
pinMode(encodersw, INPUT);
digitalWrite(senzor1, HIGH);
digitalWrite(senzor2, HIGH);
digitalWrite(encodersw, HIGH);

zero = EEPROM.read(adresa - 1); // variable for write initial values in EEPROM
if (zero != adresacorecta)
{
  EEPROM.update(adresa - 1, adresacorecta);  // zero
  EEPROM.update(adresa, 7); // initial value, 1...7 (see color)
  EEPROM.update(adresa+1, romana);
  EEPROM.update(adresa+2, br0);
  EEPROM.update(adresa+3, br);
  EEPROM.update(adresa+4, pauzamica);
  EEPROM.update(adresa+5, tpasteptate/1000);
  EEPROM.update(adresa+6, ledsmax);  
}   
  
// read EEPROM memory;
culoare = EEPROM.read(adresa);  // value: 1..7 (max)
if ((culoare > 7) or (culoare == 0)) culoare = 7;  // white


     
romana = EEPROM.read(adresa+1);
br0 = EEPROM.read(adresa+2);
br = EEPROM.read(adresa+3);
pauzamica = EEPROM.read(adresa+4);
setpausemaxtemp = EEPROM.read(adresa+5);
tpasteptate = setpausemaxtemp * 1000;
ledsmax = EEPROM.read(adresa+6);  // maximum 255 in this version
  
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

pixels.clear(); // Set all pixel colors to 'off'
lightsoff();

lcd.init();  // initialize the lcd  
lcd.createChar(0, shtop);
lcd.createChar(1, inainte);
lcd.createChar(2, inapoi);
            
  lcd.backlight();
  lcd.setCursor(0,0);
  if (romana == 1) 
  {
  lcd.print(" Lumini dinamice");
  lcd.setCursor(0,1);
  lcd.print("pentru hol/scara");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("program original");
  lcd.setCursor(0,1);
  lcd.print("ver.1.5.a/niq_ro");
  }
  else
  {
  lcd.print(" Dynamic lights ");
  lcd.setCursor(0,1);
  lcd.print("for lobby/stairs");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("  original sw.  ");
  lcd.setCursor(0,1);
  lcd.print("by niq_ro/v.1.5a");
  }
  delay(2000);
lcd.clear();

delay(500);
tptrigger = millis() - tpasteptate;

 be = culoare/4;
 ge = (culoare%4)/2;
 er = (culoare%4)%2;
 Serial.print("red = ");
 Serial.print(er);
 Serial.print(", green = ");
 Serial.print(ge);
 Serial.print(", blue = ");
 Serial.println(be);
}

void loop() {

pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength < pushlengthset) // short push
    {
    culoare++;
    if (culoare > 7) culoare = 1;
    delay(500);
    be = culoare/4;
    ge = (culoare%4)/2;
    er = (culoare%4)%2;
    state0 = 8; 
    EEPROM.update(adresa, culoare); // value for color is store in eeprom 
    Serial.print("red = ");
    Serial.print(er);
    Serial.print(", green = ");
    Serial.print(ge);
    Serial.print(", blue = ");
    Serial.println(be);
    }
         
 //This runs the setclock routine if the knob is pushed for a long time
   if (pushlength > pushlengthset)
      {
       lcd.clear();
       alloff();
       meniu();
       pushlength = pushlengthset;
       state0 = 8;
       state = 0;
      };



if (state == 0)
{
  if (digitalRead(senzor1) == 0) 
  {
    Serial.println("-1-");
    state = 121;
    tptrigger = millis();   // store the trigger the one of sensor
  }
  if (digitalRead(senzor2) == 0) 
  {
    Serial.println("-2-");
    state = 211;
    tptrigger = millis();   // store the trigger the one of sensor
  }
}

if (millis() > tptrigger + tpasteptate)
{
  //state = 0; 
  if (state0 == 121) state = 210;
  if (state0 == 211) state = 120;
}

if (state0 == 121)
{
  if (digitalRead(senzor2) == 0) 
  {
    Serial.println("-2-");
    tptrigger = millis();   // store the trigger the one of sensor
    delay(100);
    state = 122;
  }
   if (digitalRead(senzor1) == 0) 
  {
    Serial.println("-1-");
    tptrigger = millis();   // store the trigger the one of sensor
    delay(100);
  }
}

if (state0 == 211)
{
  if (digitalRead(senzor1) == 0) 
  {
    Serial.println("-1-");
    tptrigger = millis();   // store the trigger the one of sensor
    delay(100);
    state = 212;      
  }
     if (digitalRead(senzor2) == 0) 
  {
    Serial.println("-2-");
    tptrigger = millis();   // store the trigger the one of sensor
    delay(100);
  }
}

if (state0 != state) 
{
  Serial.println(state);
if (state == 1) 
{
  lcd.setCursor(14,0);
  lcd.print("*");
  allon();
}
if (state == 121) 
{
  lcd.setCursor(14,0);
  lcd.write(byte(1));
  on12();
  lcd.setCursor(14,0);
  lcd.print("*");
}
if (state == 211)
{
  lcd.setCursor(14,0);
  lcd.write(byte(2));
  on21();
  lcd.setCursor(14,0);
  lcd.print("*");
}

  if (state == 0) 
  {
    lcd.setCursor(14,0);
    lcd.write(byte(0));
    alloff();
  }
  if (state == 120) 
    {
    lcd.setCursor(14,0);
    lcd.write(byte(1));
    off12();
    lcd.setCursor(14,0);
    lcd.write(byte(0));
    state0 = state; 
    state = 0;
    }
  if (state == 210) 
    {
    lcd.setCursor(14,0);
    lcd.write(byte(2));
    off21();  
    lcd.setCursor(14,0);
    lcd.write(byte(0));
    state0 = state; 
    state = 0;
    }
    
state0 = state;  // store actual state

  if (state == 122)
  {
    state = 211;
    Serial.println("/");
  }
   if (state == 212)
  {
    state = 121;
    Serial.println("/");
  }
}

delay(1);

tptooff = (- millis() + tptrigger + tpasteptate)/1000;
if ((tptooff != tptooff0) and (tptooff >0))
{  
Serial.print("time until lights goes to off [s]: ");
Serial.println(tptooff);
tptooff0 = tptooff;
}

} // end main loop


void on12()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=0; i<ledsmax; i++) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br*er, br*ge, br*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(pauzamica); // Pause before next pass through loop
  }  
}

void on21()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=ledsmax-1; i>=0; i--) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br*er, br*ge, br*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(pauzamica); // Pause before next pass through loop
  }  
}


void off12()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=0; i<ledsmax; i++) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br0*er, br0*ge, br0*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(pauzamica); // Pause before next pass through loop
  }  
}

void off21()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=ledsmax-1; i>=0; i--) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br0*er, br0*ge, br0*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(pauzamica); // Pause before next pass through loop
  }  
}

void alloff()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=ledsmax-1; i>=0; i--) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br0*er, br0*ge, br0*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delayMicroseconds(100); // Pause before next pass through loop
  }  
}

void lightsoff()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=NUMPIXELS-1; i>=0; i--) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delayMicroseconds(100); // Pause before next pass through loop
  }  
}

void allon()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=ledsmax-1; i>=0; i--) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br*er, br*ge, br*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delayMicroseconds(100); // Pause before next pass through loop
  }  
}

// subroutine to return the length of the button push.
int getpushlength() 
{
  buttonstate = digitalRead(encodersw);  
       if(buttonstate == LOW && buttonflag==false) 
          {     
              pushstart = millis();
              buttonflag = true;
          };
          
       if (buttonstate == HIGH && buttonflag==true) 
          {
         pushstop = millis ();
         pushlength = pushstop - pushstart;
         buttonflag = false;
         lcd.setCursor (15,1);
         lcd.print (" ");
          };
       
       if(buttonstate == LOW && buttonflag == true) // https://nicuflorica.blogspot.com/2020/04/indicare-apasare-buton-apasare-scurta.html
          {  // if button is still push
          durata1 = millis() - pushstart;
          if (durata1 > pushlengthset) 
          {   
           lcd.setCursor (15,1);
           lcd.print ("*");
          }
          }
       return pushlength;
}

//menu
void meniu ()
{
  setlimbatemp = romana;
  setbrmintemp = br0;
  setbrmaxtemp = br;
  setpausemintemp = pauzamica;
  setpausemaxtemp = tpasteptate/1000;
  setledstemp = ledsmax;
 
   setlimba();  // language
   lcd.clear();
   delay(1000);
   //off();
   setbrmin();  // minimum brightness (background/standby)
   lcd.clear();
   delay(1000);
   allon();
   setbrmax();  // maximum brightness (active)
   lcd.clear();
   delay(1000);
   setpausemin(); // delay between transitions
   lcd.clear();
   delay(1000);
   setpausemax(); // waitting time
   lcd.clear();
   delay(1000);
   setledsmax(); // number of leds
   lcd.clear();
   delay(1000);
   pixels.clear(); // Set all pixel colors to 'off'

romana = setlimbatemp;
br0 = setbrmintemp;
br = setbrmaxtemp;
pauzamica = setpausemintemp;
tpasteptate = setpausemaxtemp * 1000;
ledsmax = setledstemp;
//if (ledsmax != setledstemp)
//{
//  alloff();
//lightsoff();
//  delay(500);
//NUMPIXELS = setledstemp;
//Adafruit_NeoPixel pixels(setledstemp, PIN, NEO_GRB + NEO_KHZ800);
//  allon();
//}
pixels.clear(); // Set all pixel colors to 'off'

  EEPROM.update(adresa+1, romana);
  EEPROM.update(adresa+2, br0);
  EEPROM.update(adresa+3, br);
  EEPROM.update(adresa+4, pauzamica);
  EEPROM.update(adresa+5, setpausemaxtemp);
  EEPROM.update(adresa+6, ledsmax);
}

// language
int setlimba() 
{
    lcd.setCursor (0,0);
    lcd.print ("Language / Limba");
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setlimbatemp;
    }

    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setlimbatemp = setlimbatemp + knobval;
    if (setlimbatemp < 0) 
    {
      setlimbatemp = 1;
    }
    if (setlimbatemp > 1) 
    {
      setlimbatemp = 0;
    }
    
    lcd.print (setlimbatemp);
    if (setlimbatemp == 0)
    lcd.print(" - english "); 
    else
    lcd.print(" - romana  "); 
    setlimba();
}

// minimum brightness (typ.5 / 255)
int setbrmin() 
{
    lcd.setCursor (0,0);
    if (setlimbatemp == 1) 
      lcd.print("Lumina fundal:  ");
    else 
      lcd.print("Background light");  
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setbrmintemp;
    }
    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setbrmintemp = setbrmintemp + knobval;
    if (setbrmintemp < 0) 
    {
      setbrmintemp = 0;
    }
    if (setbrmintemp > 25) 
    {
      setbrmintemp = 25;
    }
    if (knobval != 0)
    {
      br0 = setbrmintemp;
      alloff();
    }
    if (setbrmintemp < 100) lcd.print(" ");
    if (setbrmintemp < 10) lcd.print(" ");
    lcd.print (setbrmintemp);
    lcd.print("/255");
    setbrmin();
}

// maximun brightness (typ.100 / 255)
int setbrmax() 
{
    lcd.setCursor (0,0);
    if (setlimbatemp == 1) 
      lcd.print("Lumina aprinsa: ");
    else 
      lcd.print("Active light:   ");  
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setbrmintemp;
    }
    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setbrmaxtemp = setbrmaxtemp + knobval;
    if (setbrmaxtemp < 25) 
    {
      setbrmaxtemp = 25;
    }
    if (setbrmaxtemp > 100) 
    {
      setbrmaxtemp = 100;
    }
    if (knobval != 0)
    {
      br = setbrmaxtemp;
      allon();
    }
    if (setbrmaxtemp < 100) lcd.print(" ");
    if (setbrmaxtemp < 10) lcd.print(" ");
    lcd.print (setbrmaxtemp);
    lcd.print("/255");
    setbrmax();
}

// pause between change the light of each led
int setpausemin() 
{
    lcd.setCursor (0,0);
    if (setlimbatemp == 1) 
      lcd.print("Pauza tranzitii:");
    else 
      lcd.print("Transition time:");  
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setpausemintemp;
    }
    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setpausemintemp = setpausemintemp + knobval;
    if (setpausemintemp < 2) 
    {
      setpausemintemp = 2;
    }
    if (setpausemintemp > 50) 
    {
      setbrmintemp = 25;
    }
    if (knobval != 0)
    {
      pauzamica = setpausemintemp;
      on12();
      off12();
    }
    if (setpausemintemp < 100) lcd.print(" ");
    if (setpausemintemp < 10) lcd.print(" ");
    lcd.print (setpausemintemp);
    lcd.print("ms (2..50ms)");
    setpausemin();
}

// time to stay active
int setpausemax() 
{
    lcd.setCursor (0,0);
    if (setlimbatemp == 1) 
      lcd.print("Timp asteptare: ");
    else 
      lcd.print("Waitting time:  ");  
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setpausemintemp;
    }
    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setpausemaxtemp = setpausemaxtemp + knobval;
    if (setpausemaxtemp < 5) 
    {
      setpausemaxtemp = 5;
    }
    if (setpausemaxtemp > 50) 
    {
      setpausemaxtemp = 50;
    }
    if (knobval != 0)
    {
      pauzamica = setpausemintemp;
      tpasteptate = setpausemaxtemp *1000;
    //  on12();
    //  delay(tpasteptate);
    //  off12();
    }
    if (setpausemaxtemp < 100) lcd.print(" ");
    if (setpausemaxtemp < 10) lcd.print(" ");
    lcd.print (setpausemaxtemp);
    lcd.print("s (5..50s)");
    setpausemax();
}

// number of leds in strip
int setledsmax() 
{
    lcd.setCursor (0,0);
    if (setlimbatemp == 1) 
      lcd.print("Numar leduri:   ");
    else 
      lcd.print("No.leds in strip");  
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setpausemintemp;
    }
    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setledstemp = setledstemp + knobval;
    if (setledstemp < 5) 
    {
      setledstemp = 255;
    }
    if (setledstemp > 255) 
    {
      setledstemp = 5;
    }
    if (knobval != 0)
    {
    //  off12();
    }
    if (setledstemp  < 100) lcd.print(" ");
    if (setledstemp  < 10) lcd.print(" ");
    lcd.print (setledstemp );
    lcd.print("LEDs (5..255)");
    setledsmax();
}
