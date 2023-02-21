/**************************************************************************/
/*! 
    @file     Human circuit
    @author   Martin CORNU
    @date     12/02/23
    @version  1.0

This program read an analog value from a conductor touched by a human.
If the analog value is below the defined threshold it means that the human is touching the conductor.
If the human touch > 90s then deactivate an EM. If not touch during the 90s, the start again.
During touching, activate a taser (during 4s) every 20s.
An emergency button also allows the EM to be deactivated manually.
*/
/**************************************************************************/

#define DEBUG

#define LED_SUCCESS_PIN         13       // green led ON if success, OFF otherwise
#define LED_TASER_PIN           8        // yellow led ON if taser ON, OFF otherwise

#define TOUCH_PIN           A0        // pin where we get signal from human touch
#define EM_PIN              2         // electromagnet pin
#define TASER_PIN           3         // taser pin
#define BTN_EMERGENCY_PIN   4         // emergency button pin

#define TOUCHING_THR        1000      // below this threshold means human is touching
#define TOUCHING_TIME       90000     // time in ms to stay below the threshold to consider win
#define RELEASE_TIME        600       // time in ms to confirm human has released conductors (not touching)

#define TASER_DUTY          20000     // taser activation duty cycle in ms
#define TASER_DURATION      4000      // taser activation duration

#define DELAY_LOOP_MS       100

uint8_t g_u8Success = 0u;             // Flag to indicate a success
uint8_t g_u8Emergency = 0u;             // Flag to indicate an emergency fired

uint8_t g_u8EMDefaultState = HIGH;    //EM ON at boot (locked)

int g_s32ButtonState;             // the current reading from the input pin
int g_s32LastButtonState = LOW;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long g_u64LastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long g_u64DebounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  #ifdef DEBUG
    Serial.begin(9600);                     // sets serial port for communication
    Serial.println("Program start..");      // print the value to the serial port 
  #endif
  
  pinMode(LED_SUCCESS_PIN, OUTPUT);
  digitalWrite(LED_SUCCESS_PIN, LOW);

  pinMode(EM_PIN, OUTPUT);
  digitalWrite(EM_PIN, g_u8EMDefaultState);

  pinMode(TASER_PIN, OUTPUT);
  digitalWrite(TASER_PIN, LOW);

  pinMode(LED_TASER_PIN, OUTPUT);
  digitalWrite(LED_TASER_PIN, LOW);

  pinMode(TOUCH_PIN, INPUT);
  pinMode(BTN_EMERGENCY_PIN, INPUT);
}

void loop() {
  uint16_t l_u16TouchValue = 0;             // result of reading the analog pin
  
  static uint32_t l_u32Timer = 0;           // time below thr in ms
  static uint32_t l_u32ReleaseTimer = 0u;   // time release in ms
  
  static uint8_t l_u8TaserState = 0u;
  static uint32_t l_u32TaserTimeout = 0u;
  static uint32_t l_u32TaserTimer = 0u;

  static uint8_t l_u8IsTouching = 0u;

  // Btn emergency management
  BtnEmergencyManagement();

  // Shutdown taser if duration terminated or if released
  if ( (1u == l_u8TaserState) && ((0u == l_u8IsTouching) || (millis() >= l_u32TaserTimeout)) )
  {
    digitalWrite(TASER_PIN, LOW);
    digitalWrite(LED_TASER_PIN, LOW);
    l_u8TaserState = 0u;
    l_u32TaserTimeout = 0u;

    #ifdef DEBUG
      Serial.println("Taser OFF !");
    #endif
  }
  
  // Human touching measurement
  l_u16TouchValue = analogRead(TOUCH_PIN);  // read the value from human touch
#ifdef DEBUG
  Serial.println(l_u16TouchValue);          // print the value to the serial port
#endif

  // human touching detected
  if (l_u16TouchValue < TOUCHING_THR)
  {
    l_u32ReleaseTimer = 0u; // Human is touching so clear release timer
    l_u8IsTouching = 1u;
    
    // we stayed more than timeout with touching detected
    if (l_u32Timer >= TOUCHING_TIME)
    {
      g_u8Success = 1u;
    }
    // taser activation
    else if (l_u32TaserTimer >= TASER_DUTY)
    {      
      l_u32TaserTimer = 0u;
      
      l_u32TaserTimeout = millis() + TASER_DURATION;
      l_u8TaserState = 1u;
      digitalWrite(TASER_PIN, HIGH);
      digitalWrite(LED_TASER_PIN, HIGH);

      #ifdef DEBUG
      Serial.println("Taser ON !");
      #endif
    }

    l_u32Timer += DELAY_LOOP_MS;
    l_u32TaserTimer += DELAY_LOOP_MS;
  }
  // not touching
  else
  {
    // hysteresis to avoid noise / undesired release
    if (l_u32ReleaseTimer >= RELEASE_TIME)
    {
      // touching not detected then restart timer
      l_u32Timer = 0;
      l_u8IsTouching = 0u;

      #ifdef DEBUG
      Serial.println("Release");
      #endif
    }

    l_u32ReleaseTimer += DELAY_LOOP_MS;
  }

  if ((1u == g_u8Emergency) || (1u == g_u8Success))
  {
    digitalWrite(EM_PIN, !g_u8EMDefaultState);
    digitalWrite(LED_SUCCESS_PIN, HIGH);  
    digitalWrite(TASER_PIN, LOW); // Shutdown taser before end of program
    digitalWrite(LED_TASER_PIN, LOW);

  #ifdef DEBUG
    if (1u == g_u8Emergency)
    {
      Serial.println("Emergency fired ! Stop program like a success.");
    }
    else if (1u == g_u8Success)
    {
      Serial.println("Success !");
    }
  #endif
    
    while(1); // End of program
  }
  
  delay(DELAY_LOOP_MS);
}

void BtnEmergencyManagement(void) {
   int l_s32Reading = 0;                 // btn emergency reading
  
  // read the state of the switch into a local variable:
  l_s32Reading = digitalRead(BTN_EMERGENCY_PIN);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (l_s32Reading != g_s32LastButtonState) {
    // reset the debouncing timer
    g_u64LastDebounceTime = millis();
  }

  if ((millis() - g_u64LastDebounceTime) > g_u64DebounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (l_s32Reading != g_s32ButtonState) {
      g_s32ButtonState = l_s32Reading;

      // button state is high
      if (g_s32ButtonState == HIGH) {
        g_u8Emergency = 1u;
      }
    }
  }

  // save the reading. Next time through the loop, it'll be the g_s32LastButtonState:
  g_s32LastButtonState = l_s32Reading;
}
