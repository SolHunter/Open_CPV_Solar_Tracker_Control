/*
 * PWM MOTOR CONTROL ROUTINE
 * WORKS WITH A TIMER COUNTER OVERFLOW INTERRUPT
 * Overflow toggles between PWM on and of state
 * different cases of move_pwm_state distinguish between idle, ramps and plateau
 * 16 steps in duty cycle
 */

ISR(TIMER1_OVF_vect)        // motor control
{  
  switch(move_pwm_state){
    case 0: TCNT1 = 65520; break;                                                     // idle, ready for new command, wait a full cycle of 16 counts up to 65536
    case 1:                                                                           // PWM active, ramping up
            if ((!(move_speed_now>0))&&(!(move_speed_now<17))){move_speed_now = 1;}   // ensure valid move_speed_now value
            TCNT1 = 65536 - move_speed_now;                                           // preset counter --> active plateau time until overflow at 65536
            PORTB = (PORTB & PORTB_mask) | move_portb_[2+2*move_direction];           // set PORTS to PWM active states 
            PORTD = (PORTD & PORTD_mask) | move_portd_[2+2*move_direction];           // PORTB_mask, PORTD_mask make sure that settings of other digital pins wont change
            move_pwm_state = 2;                                                       // next overflow will go to case 2
            break;
    case 2:                                                                           // PWM free wheeling, ramping up
            TCNT1 = 65536 - 16 + move_speed_now;                                      // preset the counter for overflow at 65536
            PORTB = (PORTB & PORTB_mask) | move_portb_[1+2*move_direction];           // set ports to PWM free wheeling
            PORTD = (PORTD & PORTD_mask) | move_portd_[1+2*move_direction];            
            move_repeated +=1;                                                        // increase counter for repetitions of plateau ramp up
            if (move_repeated == move_ramp_repeat_[move_direction]){                  // present number of repetitions in ramp up state reached
              move_speed_now +=1;                                                     // next higher PWM duty cycle
              if (move_speed_now >= move_speed_[move_direction + 4 * move_step]){     // if plateau PWM duty cycle is reached
                move_speed_now = move_speed_[move_direction + 4* move_step];          // probably unnecessary
                move_pwm_state = 3;                                                   // got to plateau
              }
              else{
                move_pwm_state = 1;                                                   // continue ramp with next higher PWM duty cycle
              }
              move_repeated=0;                                                        // reset counter
            }
            else{
              move_pwm_state = 1;                                                     // continue ramp with the same PWM duty cycle
            }
            break;
    case 3: TCNT1 = 65536 - move_speed_now;                                           // PWM active, plateau
          PORTB = (PORTB & PORTB_mask) | move_portb_[2+2*move_direction]; 
          PORTD = (PORTD & PORTD_mask) | move_portd_[2+2*move_direction];           
          if(move_speed_now == 16){TCNT1=65528;}                                      // special case for full speed, no free wheeling time
          move_pwm_state = 4; 
          break;   
    case 4: TCNT1 = 65536 - 16 + move_speed_now;                                      // PWM free wheeling, plateau
            PORTB = (PORTB & PORTB_mask) | move_portb_[1+2*move_direction];
            PORTD = (PORTD & PORTD_mask) | move_portd_[1+2*move_direction];            
            if(move_speed_now == 16){
              TCNT1=65528; 
              PORTB = (PORTB & PORTB_mask) | move_portb_[2+2*move_direction];
              PORTD = (PORTD & PORTD_mask) | move_portd_[2+2*move_direction];
            } // special case for full speed, no free wheeling time
            move_repeated +=1;
            if (move_keep_running == 1) {move_repeated = 0; move_keep_running = 0;}  // reset plateau counter to keep running
            if (move_keep_running == 2) {
              move_repeated = move_plateau_repeat; move_keep_running = 0;
            }                                                                        // emergency stop. Presently not used in main loop         
            if (move_repeated == move_plateau_repeat){                               // end of plateau time has been reached

                move_pwm_state = 5;                                                  // start ramping down in state 5
                if(move_speed_now>1){move_speed_now -=1;}                            // decrease duty cycle
                move_repeated=0;                                                     // reset repeat counter

            }
            else{
              move_pwm_state = 3;                                                    // go to plateau with active PWM
            }
            break;      
    case 5:                                                                          // PWM active, ramping down 
            TCNT1 = 65536 - move_speed_now;                                                             
            PORTB = (PORTB & PORTB_mask) | move_portb_[2+2*move_direction]; 
            PORTD = (PORTD & PORTD_mask) | move_portd_[2+2*move_direction]; 
            move_pwm_state = 6; 
            break;
    case 6: TCNT1 = 65536 - 16 + move_speed_now;                                        // PWM free wheeling, ramping down
            PORTB = (PORTB & PORTB_mask) | move_portb_[1+2*move_direction];
            PORTD = (PORTD & PORTD_mask) | move_portd_[1+2*move_direction];
            move_repeated +=1;
            if (move_repeated == move_ramp_repeat_[move_direction]){
              if (move_speed_now == 1){
                move_pwm_state = 7;
              }
              else{
                move_pwm_state = 5;
                move_speed_now -=1;
              }
              move_repeated=0;
            }
            else{
              move_pwm_state = 5;              
            }
            break;
    case 7: TCNT1 = 65536 - 16;                                                                         // brake
            move_repeated += 1;            
            if (move_repeated == move_brake_repeat){
              move_pwm_state = 0;
              PORTB = (PORTB & PORTB_mask);                                                             // all motor control pins low
              PORTD = (PORTD & PORTD_mask);
              move_repeated = 0;
              last_micros_move=micros();
            }            
            break;
  }
//  Serial.print(move_speed_now);   Serial.println(move_pwm_state);    
}    

// READ PARAMETERS FROM EEPROM
// reading from EEPROM is comparably slow, should not be done in ISR
void read_EEPROM_parameters(){
  move_speed_[0] = ERI(16); //3; //up
  move_speed_[1] = ERI(18);  //8; //down
  move_speed_[2] = ERI(20); //4; //left
  move_speed_[3] = ERI(20);  //4; //right
  move_speed_[4] = ERI(22);  //8; //up step
  move_speed_[5] = ERI(24);  //12; //down step
  move_speed_[6] = ERI(26);  //8; //left step
  move_speed_[7] = ERI(26);  //8; //right step

  move_ramp_repeat_[0] = ERI(28);  //1;  //up
  move_ramp_repeat_[1] = ERI(30);  //1;  //down
  move_ramp_repeat_[2] = ERI(32);  //5;  //left
  move_ramp_repeat_[3] = ERI(32);  //5;  //right

  move_plateau_repeat_[0] = ERI(34);  //5;  //up
  move_plateau_repeat_[1] = ERI(36);  //5;  //down
  move_plateau_repeat_[2] = ERI(38);  //20;  //left
  move_plateau_repeat_[3] = ERI(38);  //20;  //right
  move_plateau_repeat_[4] = ERI(40);  //1000;  // for continous moves, shuts off after 500ms
  
  move_brake_repeat = ERI(14);  // brake repeat
}

// MEASURING THE WIND SPEED WITH REED RELAY WIND WHEEL
// not fully tested yet, need to find right value for debounce time
void debounceInterrupt() {
  if((long)(micros() - last_micros) >= debouncing_time) {
    wind_speed = 10000000 / (micros() - last_micros);  // clicks per 10 seconds
    last_micros = micros();
  }
}
