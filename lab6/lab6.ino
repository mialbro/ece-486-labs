/*Author: Mark Robinson
   CWID: 11635959
   Lab: Lab 5
   Date: March 08, 2019
*/

#include <avr/wdt.h>
#include <fourDigitDisplay.h>

// Initialize 4-segment display object
fourDigitDisplay SevenSegment;


// ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
#define POLLING B10000111
#define INTERRUPT B10001111

boolean volatile readFlag = false;
float analogVal = 0;
int wdtCount = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Board Was Reset");
  reset();
  wdtCount = 0;
  watchdogSetup();  // Configures watchdog timer
  adc_init();
}

void loop() {
  String inString = "";
  float polling[] = {0, 0}; // [ pollingCount, pollingTime ]
  float analog[] = {0, 0};  // [ analogReadCount, analogReadTime]
  float interrupt[] = {0, 0};  // [ analogReadCount, analogReadTime]

  while (1) {
    while (Serial.available() > 0) {
      wdt_reset();
      wdtCount = -1;
      char inChar = Serial.read();
      if (inChar != '\n') {
        inString += (char)inChar;
      }
      // if you get a newline, print the string, then the string's value:
      else if (inChar == '\n') {
        if (inString.length() == 1 && (inString[0] == 'a' || inString[0] == 'b') | inString[0] == 'c') {
          convert(inString[0], polling, analog, interrupt);
        }
        else {
          Serial.println("Error: invalid user input - the only valid user input is 'a', 'b', or 'c')");
          Serial.println();
        }
        inString = "";
      }
    }
  }
}


/*Name: watchdogSetup
   Inputs: void
   Outputs: void
   Functions: Sets the watchdog timer to 4 seconds
*/
void watchdogSetup(void) {
  SevenSegment.display(0);
  cli();
  wdt_reset();
  // Enter Watchdog Configuration mode:
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  // Set Watchdog settings:
  WDTCSR = (1 << WDIE) | (0 << WDE) | (0 << WDP3) | (1 << WDP2) | (1 << WDP1) | (0 << WDP0);
  sei();
}

// Interrupt service routine for the ADC completion
ISR(ADC_vect){
  readFlag = true;
}

/*Name: Watchdog Timer ISR
   Inputs: WDT_vect
   Outputs: void
   Functions: resets the watchdog timer every second
              and restarts the board after 4 seconds
*/
ISR(WDT_vect) {
  wdtCount += 1;
  SevenSegment.display(wdtCount);
  if (wdtCount < 3) {
    wdt_reset();
  }
  else {
    cli();
    wdt_reset();
    // Enter Watchdog Configuration mode:
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    sei();
  }
}

/*Name: adc_int
   Inputs: void
   Outputs: void
   Functions: Initializes the analog to digital converter
*/
void adc_init() {
  // AREF = AVcc
  ADMUX = (1 << REFS0);
  // ADC Enable and prescaler of 128
  // 16000000/128 = 125000
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

void enableInterrupt() {
  ADCSRA = INTERRUPT;
}

void disableInterrupt() {
  ADCSRA = POLLING;
}

/*Name: startMessage
   Inputs: char method
   Outputs: void
   Functions: Displays the ADC method
*/
void startMessage(char method) {
  if (method == 'a')  // Notify the user that analogRead() method is being used
    Serial.println("Starting a set of conversions using analogRead()");
  else if (method == 'b') // Notify the user that the polling method is being used
    Serial.println("Starting a set of conversions using polling and port manipulation");
  else if (method == 'c') // Notify the user that the interrupt method is being used
    Serial.println("Starting a set of conversions using interrupts");
}


/*Name: reset
   Inputs: void
   Outputs: void
   Functions: Displays the welcome message
*/
void reset() {
  // Displays wlcome message informing the user of the commands
  Serial.println("Select a type of conversion to perform ('a' for lab #4, 'b' for lab #5, and 'c' for lab #6)");
  Serial.println();
  wdt_reset();
  wdtCount = -1;
  return;
}

/*Name: convert
   Inputs: void
   Outputs: void
   Functions: Reads the analog value at the potentiometer,
   converts it to a digital value, and displays the values
   in hex and the time it took to convert
*/
void convert(char method, float polling[], float analog[], float interrupt[]) {
  startMessage(method);
  float tempTime[30];
  int value = 0;
  char hex[50] = {0};
  
  for (int i = 0; i < 30; i++) {
    tempTime[i] = performADC(method, i);
    delay(500);
    wdt_reset();
    wdtCount = -1;
  }
  wdtCount = -1;
  
  ADCSRA &= (0 << ADSC);
  disableInterrupt();
  
  float avg = getAverageTime(tempTime);

  Serial.print("avg conversion time = ");
  Serial.print((float)avg, 2);
  Serial.println(" usecs");
  Serial.println();
  
  while (Serial.available() > 0) {
    Serial.read();
    wdt_reset();
  }
  updateTotals(method, avg, analog, polling, interrupt);
  totals(analog, polling, interrupt);  // Print the total results
  reset();
  wdt_reset();
}

void updateTotals(char method, float avg, float analog[], float polling[], float interrupt[]) {
  if (method == 'a') {
    analog[0] += 1.00;
    analog[1] += avg;
  }
  else if (method == 'b') {
    polling[0] += 1.00;
    polling[1] += avg;
  }
  else if (method == 'c') {
    interrupt[0] += 1.00;
    interrupt[1] += avg;
  }
}

float getAverageTime(float tempTime[]) {
  float avg = 0.0;
  for (int i = 0; i < 30; i++) {
    avg += tempTime[i];
    wdt_reset();
  }
  return avg / 30.0;
}

int performADC(char method, int count) {
  int value = 0;
  float lenTime = 0;
  
  if (method == 'a') {
     lenTime = micros();
     value = analogRead(0);
     lenTime = micros() - lenTime;
  }
  else {
    ADMUX = (ADMUX & 0xF8)|(0 & 0b00000111); // clears the bottom 3 bits before ORing
    if (method == 'b') {
      lenTime = micros();
      while(ADCSRA & (1<<ADSC));
      lenTime = micros() - lenTime;
      value = ADC;
    }
    else if (method == 'c') {
      if (count == 0)
        enableInterrupt();
      lenTime = micros();
      ADCSRA |= (1<<ADSC);
      while (readFlag == false);
      value = ADCL | (ADCH << 8);
      lenTime = micros() - lenTime;
      readFlag = false;
      if (count == 29)
        disableInterrupt();
    }
  }
  displayADC(value, count, lenTime);
  return lenTime;
}

void displayADC(int value, int count, int lenTime) {
  char hex[5] = {0};
  Serial.print("#");
  Serial.print(count + 1);
  Serial.print(":   digital value = ");
  sprintf(hex, "%03X", value);
  Serial.print(hex);
  Serial.print("    Time = ");
  Serial.print((int)lenTime);
  Serial.println(" usecs");
  Serial.println();
  return;
}


/*Name: totals
   Inputs: float, float
   Outputs: void
   Functions: Displays the total times and the average
   ADC values using the analog and polling methods
*/
void totals(float analog[], float polling[], float interrupt[]) {
  wdt_reset();
  Serial.println("TOTALS");
  Serial.print("analogRead() Average Conversion Time = ");
  if (analog[0] == 0)
    Serial.println("NA (0 total conversions)");
  else {
    Serial.print((float)analog[1] / (float)analog[0]);
    Serial.print(" usecs (");
    Serial.print((int)analog[0] * 30);
    Serial.println(" total conversions)");
  }

  Serial.print("Polling Average Conversion Time = ");
  if (polling[0] == 0)
    Serial.println("NA (0 total conversions)");
  else {
    Serial.print((float)polling[1] / (float)polling[0]);
    Serial.print(" usecs (");
    Serial.print((int)polling[0] * 30);
    Serial.println(" total conversions)");
  }
  
    Serial.print("Interrupt-driven Average Conversion Time = ");
  if (interrupt[0] == 0)
    Serial.println("NA (0 total conversions)");
  else {
    Serial.print((float)interrupt[1] / (float)interrupt[0]);
    Serial.print(" usecs (");
    Serial.print((int)interrupt[0] * 30);
    Serial.println(" total conversions)");
  }
  wdt_reset();
}
