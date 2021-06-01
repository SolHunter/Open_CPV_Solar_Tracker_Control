void setup() {
 analogReference(INTERNAL);  
 Serial.begin(19200);                                                   // setup serial, 114400 is not reliable, 19200 recommended
 pinMode(stormpin, INPUT_PULLUP);                                       // stormpin D3 to be connected to a wind wheel with reed relais or similar
 attachInterrupt(1, debounceInterrupt, FALLING);                        // Interrupt 1 at strompin D3, uses falling because it is sharper than rising.

 Serial.println(ERI(EA_azimuth_left_of_east));                          // prints 1 if tracker was switched off in left of East position 
                                                                        // East is the middle of the Azimuth range.
 if (ERI(EA_azimuth_left_of_east)>0) prev_azimuth_compass = -100;       // recover position in -1600 .. 3400 azimuth range, see compass_calculations()
 else prev_azimuth_compass = 1900;                                      // after first pass of compass_calculations the prev_compass_position will be correct
 Serial.println(prev_azimuth_compass);                                  // displays -100 or 1900 at startup 


// Arduino initializations
 while(!MPU9250_setup()) Serial.println(F("retry I2C"));

// digital outputs for driving the motor FETs on Port B = Pins 8-13
// for new electronics on a single circuit board
// move_portb_[0] = B00000000;  // half bridge off
// move_portb_[1] = B00100000;  // up free wheeling state = break upwards
// move_portb_[2] = B00100001;  // run up actively
// move_portb_[3] = B00010000;  // down free wheeling state = break downwards
// move_portb_[4] = B00010001;  // down
// move_portb_[5] = B00001000;  // left free wheeling
// move_portb_[6] = B00001010;  // left
// move_portb_[7] = B00000100;  // right free wheeling
// move_portb_[8] = B00000110;  // right

/* Dual VNH5019 shield on Arduino UNO
D2: M1INA     
D4: M1INB
D6: M1ENDiag
D7: M2INA
D8: M2INB
D9: M1PWM
D10: M2PWM
D12: M2EN/Diag
A0: M1CS
A1: M2CS            PortB                      PortD
                    D1111     
for UNO controlbox   321098                   D76543210   */
 move_portb_[0] = B00000000; move_portd_[0] = B00000000;  // half bridge off
 move_portb_[1] = B00000000; move_portd_[1] = B01000100;  // up free wheeling state = break upwards
 move_portb_[2] = B00000010; move_portd_[2] = B01000100;  // run up actively
 move_portb_[3] = B00000000; move_portd_[3] = B01010000;  // down free wheeling state = break downwards
 move_portb_[4] = B00000010; move_portd_[4] = B01010000;  // down
 move_portb_[5] = B00010000; move_portd_[5] = B10000000;  // left free wheeling
 move_portb_[6] = B00010100; move_portd_[6] = B10000000;  // left
 move_portb_[7] = B00010001; move_portd_[7] = B00000000;  // right free wheeling
 move_portb_[8] = B00010101; move_portd_[8] = B00000000;  // right 
//mask            B11101000                   B00101011   // all pins which are not motor pins are masked with a 1, see #define PORT?_mask,
                                                          // the pins that should not change during port manipulation
 read_EEPROM_parameters();

 DDRB = DDRB | (~PORTB_mask);                             // set motor control pins as outputs, leave other pins as they are
 DDRD = DDRD | (~PORTD_mask);                             // set motor control pins as outputs 
 PORTD = PORTD + B00000001<<stormpin;                     // set PULLUP on stormpin, same as .. + B00001000

 // Timer 1 for the PWM routine. Count up to 2^16=65536, then perform action on overflow
 noInterrupts();                                          // disable all interrupts for setting values
 TCNT1 = 65504;                                           // first preset of counter, wait 32 counts until 65536 before first action
 TCCR1A = 0;                                              // sets resolution for PWM, 0 works fine
 TCCR1B = 0;                                              // register for prescaler. First set to 0, then modified below

 TCCR1B |= (1 << CS12);                                   // specify 256 as Prescale-Value, 3.9 kHz PWM on 16MHz µC, 1.95 kHz on 8 MHz µC 
                                                          // see https://www.heise.de/developer/artikel/Timer-Counter-und-Interrupts-3273309.html
 TIMSK1 |= (1 << TOIE1);                                  // activate Timer Overflow Interrupt 
 interrupts();                                            // activate all Interrupts

  if (!(ERI(EA_first_start)==1234)) {
    calibration_step=1;                                   // this enables writing to all EEPROM positions
    EWI(EA_first_start,1234); 
    EWI(EA_operation_days,0);  //set day counter to 0 for first start
    for (byte hs_counter = 0; hs_counter < 64; hs_counter++)
    {
      EWI(int(hs_counter*2), int(pgm_read_word(&EEPROM_limits[hs_counter*3+2])) ); 
//      Serial.println(F("presets restored"));
    }                  
  }   
  if (!(ERI(EA_calibrated)==1234)) {calibration_step=1; EWI(EA_incomplete_recalibration,0);}// 1234 if calibrated, calibrate otherwise
  else calibration_step=0;                                                                  // normal operation
 
}
