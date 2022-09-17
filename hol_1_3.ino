// inteligent light for lobby - original by Nicu FLORICA (niq_ro)
// v.1.0 - initial
// v.1.1 - serial monitoring for time to off
// v.1.2 - changed colours (8 colors) by button
// v.1.3 - stored color in EEPROM

#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define adresa  50  // adress for store the
byte zero = 0;  // variable for control the initial read/write the eeprom memory

#define PIN       5  // Which pin on the Arduino is connected to the NeoPixels? 
                      // On Trinket or Gemma, suggest changing this to 1
                      
#define NUMPIXELS 8 // How many NeoPixels are attached to the Arduino?

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define pin1 2
#define pin2 3
#define pin3 4

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
byte br0 = 1;  // brightness
byte er = 0;  // red 
byte ge = 1;  // green
byte be = 0;  // blue

byte culoare = 7;   // 1 - red, 2 - green, 4 - blue, 7 - white 

// "on" light (0 = off, 255 = max)
byte br = 10;  // brightness

long pauzamica = 200;

int tptooff = 0;
int tptooff0 = 0;
int tptooff2 = 0;
int tptooff20 = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("-");
  Serial.println("Inteligent lights for lobby v.1.3");
  Serial.println("original software wrote by Nicu FLORICA (niq_ro)");
  Serial.println("-");
pinMode(pin1, INPUT);
pinMode(pin2, INPUT);
pinMode(pin3, INPUT);
//digitalWrite(pin1, HIGH);
//digitalWrite(pin2, HIGH);
//digitalWrite(pin3, HIGH);

zero = EEPROM.read(adresa - 1); // variable for write initial values in EEPROM
if (zero != 19)
{
EEPROM.update(adresa - 1, 19);  // zero
EEPROM.update(adresa, 7); // initial value, 1...7 (see color)
}   
  
// read EEPROM memory;
culoare = EEPROM.read(adresa);  // value: 1..7 (max)
if ((culoare > 7) or (culoare == 0)) culoare = 7;  // white

  
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

pixels.clear(); // Set all pixel colors to 'off'
alloff();
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

if (digitalRead(pin3) == 0) 
  {
    culoare++;
    if (culoare > 7) culoare = 1;
    delay(500);
    be = culoare/4;
    ge = (culoare%4)/2;
    er = (culoare%4)%2;
    state0 = 7; 
    EEPROM.update(adresa, culoare); // value for color is store in eeprom 
    Serial.print("red = ");
    Serial.print(er);
    Serial.print(", green = ");
    Serial.print(ge);
    Serial.print(", blue = ");
    Serial.println(be);
  }

if (state == 0)
{
  if (digitalRead(pin1) == 0) 
  {
    Serial.println("-1-");
    state = 121;
    tptrigger = millis();   // store the trigger the one of sensor
  }
  if (digitalRead(pin2) == 0) 
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
  if (digitalRead(pin2) == 0) 
  {
    Serial.println("-2-");
    tptrigger = millis();   // store the trigger the one of sensor
    delay(100);
    state = 122;
  }
   if (digitalRead(pin1) == 0) 
  {
    Serial.println("-1-");
    tptrigger = millis();   // store the trigger the one of sensor
    delay(100);
  }
}

if (state0 == 211)
{
  if (digitalRead(pin1) == 0) 
  {
    Serial.println("-1-");
    tptrigger = millis();   // store the trigger the one of sensor
    delay(100);
    state = 212;      
  }
     if (digitalRead(pin2) == 0) 
  {
    Serial.println("-2-");
    tptrigger = millis();   // store the trigger the one of sensor
    delay(100);
  }
}

if (state0 != state) 
{
  Serial.println(state);
if (state == 1) allon();
if (state == 121) on12();
if (state == 211) on21();

  if (state == 0) alloff();
  if (state == 120) 
    {
    off12();
    state0 = state; 
    state = 0;
    }
  if (state == 210) 
    {
    off21();  
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
  for(int i=0; i<NUMPIXELS; i++) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br*er, br*ge, br*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(pauzamica); // Pause before next pass through loop
  }  
}

void on21()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=NUMPIXELS; i>=0; i--) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br*er, br*ge, br*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(pauzamica); // Pause before next pass through loop
  }  
}


void off12()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=0; i<NUMPIXELS; i++) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br0*er, br0*ge, br0*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(pauzamica); // Pause before next pass through loop
  }  
}

void off21()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=NUMPIXELS; i>=0; i--) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br0*er, br0*ge, br0*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(pauzamica); // Pause before next pass through loop
  }  
}

void alloff()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=NUMPIXELS; i>=0; i--) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br0*er, br0*ge, br0*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(1); // Pause before next pass through loop
  }  
}

void allon()
{
//  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=NUMPIXELS; i>=0; i--) // For each pixel...
  { 
    pixels.setPixelColor(i, pixels.Color(br*er, br*ge, br*be));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(1); // Pause before next pass through loop
  }  
}
