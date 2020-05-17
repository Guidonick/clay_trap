#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <printf.h>

RF24 radio(9,10);

const int role_pin = A4;

const uint8_t button_pins[] = { 2,3,4,5,6,7,8,A0 };
const uint8_t num_button_pins = sizeof(button_pins);
const uint8_t led_pins[] = { 2,3,4,5,6,7,8,A0 };
const uint8_t num_led_pins = sizeof(led_pins);

// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

// Role management
// Set up role. If A4 is grounded the role is Receiver. 

typedef enum { role_remote = 1, role_led } role_e;

const char* role_friendly_name[] = { "invalid", "Remote", "LED Board"};

// The role of the current running sketch 
role_e role;
uint8_t button_states[num_button_pins];
uint8_t led_states[num_led_pins];
uint8_t button_states_prev[num_button_pins]; 
uint8_t led_states_prev[num_led_pins];

void setup(void)
{
  // set up the role pin
  pinMode(role_pin, INPUT);
  digitalWrite(role_pin,HIGH);
  delay(20);

  // read the address pin, establish our role
  if ( digitalRead(role_pin) )
    role = role_remote;
  else
    role = role_led;

  Serial.begin(9600);
  printf_begin();
  printf("\n\rRF24/examples/led_remote/\n\r");
  printf("ROLE: %s\n\r",role_friendly_name[role]);

  // Setup and configure rf radio

  radio.begin();

  if ( role == role_remote )
  {
    radio.openWritingPipe(pipe);
  }
  else
  {
    radio.openReadingPipe(1,pipe);
  }

  if ( role == role_led )
    radio.startListening();

  radio.printDetails();

  // Set up buttons / LED's

  if ( role == role_remote )
  {
    int i = num_button_pins;
    while(i--)
    {
      pinMode(button_pins[i],INPUT);
      digitalWrite(button_pins[i],HIGH);
    }
  }

  // Turn LED's ON until we start getting keys
  if ( role == role_led )
  {
    int i = num_led_pins;
    while(i--)
    {
      pinMode(led_pins[i],OUTPUT);
      led_states[i] = HIGH;
      digitalWrite(led_pins[i],led_states[i]);
    }
  }

}

// Loop

void loop(void)
{

  if ( role == role_remote )
  {
    // Get the current state of buttons, and test if the current state is different from the last state we sent
    int i = num_button_pins;
    bool different = false;
    while(i--)
    {
      uint8_t state = ! digitalRead(button_pins[i]);
      if ( state != button_states[i] )
      {
        different = true;
        button_states[i] = state;
      
      }
    }

    // Send the state of the buttons to the LED board
    if ( different )
    {
      printf("Now sending...");
      bool ok = radio.write( button_states, num_button_pins );
      if (ok)
        printf("ok\n\r");
      else
        printf("failed\n\r");
    }
    delay(20);
  }

  // LED role.  Receive the state of all buttons, and reflect that in the LEDs
 
  if ( role == role_led )
  {
    // if there is data ready
    if ( radio.available() )
    {
      while (radio.available())
      {
        // Fetch the payload, and see if this was the last one.
        radio.read( button_states, num_button_pins );
        printf("Got buttons\n\r");

        // For each button, if the button now on, then toggle the LED
        int i = num_led_pins;
        
        while(i--)
        {
          if ( button_states[i] )
          {
            led_states[i] ^= HIGH;
            digitalWrite(led_pins[i],led_states[i]);
          }
        }
      }
    }
  }
}
