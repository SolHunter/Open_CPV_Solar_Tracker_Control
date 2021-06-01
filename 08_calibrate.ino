/* CALIBRATION ROUTINE
 * This is the main innovation within this software. 
 * 
 * LIST OF CALIBRATION STEPS
 * 
 * USER DEFINED CALIBRATION
 * 1&2: The user sets the parameters in the EEPROM
 * 3: steer the tracker to the sun, let it enter the sun sensor tracking mode 
 *          and adjust the mounting screws of the control box.
 * 4: manual correction of the uppermost elevation position
 * 5&6: manual movement to the west
 * 7&8: manual correction of the lowermost elevation position
 * 
 * START OF AUTOMATIC CALIBRATION
 * 9-10-9-... measure west magnetic reading for discrete elevation positions
 * 11..19 loop through 11..19 for full azimuth sweeps in alternate directions at discrete elevation positions
 *        measure the minimum and maximum my and mx_horizontal readings to calculate neutral positions and ranges
 * 20: calibration complete, resume tracking after recalibration
 * 21: wait for restart after initial calibration
 * 
 * RECALIBRATION
 * 22, 23: prepare for recalibration
 * 24: copy EEPROM positions to upper EEPROM range for recovery from interrupted recalibration, continue with o
 * 
 * STORM DETECTION AND STOW
 * 30&31
 * 
 */




void calibrate(){
//  static byte on_target_counter=0;                  // The calibration routine does NOT stop the main loop. Therefore variables needed in the next pass must be static or global

  // PRINT CALIBRATION STEP
  if (prev_calibration_step != calibration_step){Serial.print(F("calibration_step ")); Serial.print(calibration_step); Serial.print(F(" ")); Serial.println(millis()); prev_calibration_step = calibration_step; }

  
  // AVERAGE COMPASS READING FOR BETTER PRECISION
  if ((calibration_step > 10)&(calibration_step<20)){ 
    for (i=0; i<9; i++){                            // shift array positions for moving average (maybe a pointer would be more efficient)
      mx_[i]=mx_[i+1];
      my_[i]=my_[i+1];
      mz_[i]=mz_[i+1];  
    }
    mx_[9]=mx_raw;
    my_[9]=my_raw;
    mz_[9]=mz_raw;
    mx_av=0;
    my_av=0;
    mz_av=0;  
    for (i=0; i<10; i++){
      mx_av+=mx_[i]/10;
      my_av+=my_[i]/10;
      mz_av+=mz_[i]/10;
    }
    mx = mx_av;
    mz = mz_av;
    calc_mx_horizontal();   
  }

//  FIND MAX MIN MAGN. READINGS FOR mx_horizontal, mz
  if ((calibration_step > 11)&&(calibration_step < 20))
  {     


    if (mx_horizontal > mx_horizontal_max){mx_horizontal_max = mx_horizontal;}
    if (my_av > my_max){my_max = my_av;}
    if (mx_horizontal < mx_horizontal_min){mx_horizontal_min = mx_horizontal;}
    if (my_av < my_min){my_min = my_av;}    
    if (print_compass){
      Serial.print(mx_horizontal); 
      Serial.print(F("\t mxh_max ")); Serial.print(mx_horizontal_max); 
      Serial.print(F("\t mxh_min ")); Serial.print(mx_horizontal_min);       
      Serial.print(F("\t my_max ")); Serial.print(my_max); 
      Serial.print(F("\t my_min ")); Serial.print(my_min);
    }
  }

  // 1 ASK FOR CORRECTION OF EEPROM VALUES
  if (calibration_step==1){                                
    azimuth_target=10000;                                                                 // stop azimuth 
    Serial.println(F("set EEPROM values, see handbook, / to finish"));                    // prompt user
    calibration_step=2;
    serial_mode=0;                                                                        // serial_mode=0 is used for entering values to the EEPROM
  }

  // 2 WAIT FOR USER TO FINISH ENTERING EEPROM VALUES
  if (calibration_step==2){     
    if (serial_mode==int('/')){                                                           // entering values happens in handle_serial(), wait for / to finish (cannot use y here because it is a parameter)
      calibration_step=3; 
//      on_target_counter = 0;                                                              // reset on target counter, used in step 3
      Serial.println(F("Find sun: U D L R c_ontinue s_top f_inish"));
      serial_mode=2;
    }
  }

  // 3 MANUAL MOVES same as 6

  // 4 CORRECT ZENITH
  // This is done by changing the correction values with active compass based elevation tracking
  if ((calibration_step==4)){   

    if (serial_mode == int('u')){EWI(EA_el_up_corr,ERI(EA_el_up_corr)+1);serial_mode=2;}  // increase correction value
    if (serial_mode == int('U')){EWI(EA_el_up_corr,ERI(EA_el_up_corr)+10);serial_mode=2;} // increase
    if (serial_mode == int('d')){EWI(EA_el_up_corr,ERI(EA_el_up_corr)-1);serial_mode=2;}  // decrease 
    if (serial_mode == int('D')){EWI(EA_el_up_corr,ERI(EA_el_up_corr)-10);serial_mode=2;} // decrease   
    if (serial_mode == int('f')){calibration_step = 5;} // f for finish
  }

  // 5 REQUEST WEST CORRECTION
  if ((calibration_step==5)){   
    Serial.println(F("corr West r R l L, c ontinue, s top, f"));
    calibration_step=6;
    serial_mode=2;                                                    // request single letter commands from handle_serial()
  }  

  // 6 CORRECT WEST or 3 MANUAL MOVES AND SUN TRACKING 
  if ((calibration_step==6)||(calibration_step==3)){
 //  Serial.print(serial_mode); Serial.print(" "); Serial.println(move_pwm_state);

   if ((calibration_step ==3)&!Serial.available()) {track_sensor();}                        // enable sensor tracking
  
   
   if (move_pwm_state==0){                                            // only react when motors are idle
    if (serial_mode == int('l')){                                     // left step
      move_direction = 2;
      move_plateau_repeat = move_plateau_repeat_[move_direction];     // tracking step plateau
      move_step = 1;
      move_pwm_state = 1;
    }
    if (serial_mode == int('L')){                                     // big left step
      move_direction = 2;
      move_plateau_repeat = move_plateau_repeat_[4];                  // continuous move plateau
      move_step = 0;
      move_pwm_state = 1;
    }
      if (serial_mode == int('r')){                                   // right step
      move_direction = 3;
      move_plateau_repeat = move_plateau_repeat_[move_direction];
      move_step = 1;
      move_pwm_state = 1;
    }
    if (serial_mode == int('R')){                                     // big right step
      move_direction = 3;
      move_plateau_repeat = move_plateau_repeat_[4];
      move_step = 0;
      move_pwm_state = 1;
    }  

    if (calibration_step==3){                                         // Big up and down steps for manual move to the sun
      if (serial_mode == int('U')){     
        move_direction = 0;
        move_plateau_repeat = move_plateau_repeat_[4];
        move_step = 0;
        move_pwm_state = 1;
      }
      
      if (serial_mode == int('D')){     
        move_direction = 1;
        move_plateau_repeat = move_plateau_repeat_[4];
        move_step = 0;
        move_pwm_state = 1;
      }
    }
       


   }
   if (serial_mode == int('c')){temp_byte = 1;}                           // need to store this information in temp_byte, because move_keep_running resets to 0 in ISR
   if (serial_mode == int('s')){temp_byte = 0;}
   if ((temp_byte == 1)){move_keep_running = 1;}
   if (serial_mode == int('f')){ // f for finish
     while(move_pwm_state){}
     temp_byte = 0;
     if (calibration_step == 6){
       calibration_step = 7; i=100; /*mx_zenith_north=0; my_zenith_north=0;*/
       temporary_mx_neutral = ERI(EA_mx_neutral)-mx_raw; 
       // Serial.print("temp_mx="); Serial.println(temporary_mx_neutral);
     } 
     else {
       elevation_target = ERI(EA_el_up_limit);
       Serial.print(F("corr ")); Serial.print(ERI(EA_el_up_limit)/10); Serial.println(F("° u U d D, f finish"));
       calibration_step = 4; 
       serial_mode=2;                                                    // request single letter commands from handle_serial()
       sensor_track = 0;
     }
   }   
   serial_mode=2;                                                         // wait for more single letter commands
   if (calibration_step==3) sensor_track = 1;                             // suppress goto, enable sensor tracking, this is probably redundant with main loop. Better keep it.




  
  }
    
  // 7 MOVE TO ELEVATION DOWN LIMIT
  if ((calibration_step==7)){               
    elevation_target = ERI(EA_el_down_limit);

     Serial.print(F("correct ")); Serial.print(ERI(EA_el_down_limit)/10) ;Serial.println(F("° u U d D f")); // prompt user
     calibration_step += 1; 
     serial_mode=2;
    
  }
  
  // 8 CORRECT DOWN LIMIT    
  // This is done by changing the correction values with active compass based elevation tracking
  if ((calibration_step==8)){   
    if (serial_mode == int('u')){EWI(EA_el_down_corr,ERI(EA_el_down_corr)+1);serial_mode=2;} // increase value
    if (serial_mode == int('U')){EWI(EA_el_down_corr,ERI(EA_el_down_corr)+10);serial_mode=2;} // increase value
    if (serial_mode == int('d')){EWI(EA_el_down_corr,ERI(EA_el_down_corr)-1);serial_mode=2;} // decrease value 
    if (serial_mode == int('D')){EWI(EA_el_down_corr,ERI(EA_el_down_corr)-10);serial_mode=2;} // decrease value   
    if (serial_mode == int('f')){
      i=30; calibration_step += 1; Serial.println(F("starting automatic calibration"));
      mx_west = 0;
      my_west = 0;
      mz_west = 0;
      elevation_position = 0;               // 0: elevation_down_limit, (2*ERI(EA_el_steps)): elevation_up_limit
      serial_mode = 0;                      //
      temporary_mz_neutral = ERI(EA_mz_neutral)-mz_raw; 
      // Serial.print("temp_mz="); Serial.println(temporary_mz_neutral);
    } // f for finish
  }

  // 9 MEASURE WEST MAGNETIC READINGS
  if ((calibration_step==9)){   
    if (i>0){                               // countdown from 30 and add up for averaging (float, no risk of overflow)
      mx_west += mx_raw;
      my_west += my_raw;
      mz_west += mz_raw;
      i--;
      Serial.println(i);
    }
    else {
      calibration_step += 1; 
      mx = mx_west / 30;                    // calculate average
      my_west /= 30; 
      mz = mz_west / 30; 

      calc_mx_horizontal();                 // project magnetic x-z-vector to a horizontal plane



      EWI(150+2*elevation_position, mx_horizontal);  //store west magnetometer readings for each elevation step
      EWI(200+2*elevation_position, my_west);      


      Serial.println(); Serial.println(elevation_target);
      Serial.print(F("el_pos=")); Serial.println(elevation_position);
      Serial.print(F("mx=")); Serial.println(mx);
      Serial.print(F("my=")); Serial.println(ERI(200+2*elevation_position));
      Serial.print(F("mz=")); Serial.println(mz);
      Serial.print(F("mxh=")); Serial.println(ERI(150+2*elevation_position));     

      if (elevation_position<2*ERI(EA_el_steps)){           
        elevation_position += 1; 
        Serial.print(F("el_pos_=")); Serial.println(elevation_position);        
        elevation_target = (ERI(EA_el_up_limit)*elevation_position + ERI(EA_el_down_limit)*(2*ERI(EA_el_steps)-elevation_position))/(2*ERI(EA_el_steps));
        i=30;
        mx_west = 0;                                      // presets for averaging in next pass
        my_west = 0;
        mz_west = 0;

      }
      else {
        calibration_step++;                               // Jump 1 step to 11
        move_direction = 2;                               // start rotating left
        move_plateau_repeat = move_plateau_repeat_[4];
        move_step = 0;
        move_pwm_state = 1;
        j=0;
        print_compass = 1;
        

      }
    }
    last_micros_move = micros();
  }





  // 10 19 WAIT FOR NEXT ELEVATION
  if ((calibration_step==10)|calibration_step==19){

 
//    if (move_pwm_state == 0){on_target_counter ++;}
//    else {on_target_counter = 0;}
//    if (on_target_counter > 4 + 5* (calibration_step==19) ){                // need precise readings of average values for step 19 --> wait longer
//      on_target_counter = 0;
    if ((micros()-last_micros_move > 1000000)&(move_pwm_state==0)){                                   // if the tracker hasn't moved for a second
       calibration_step -=1;

    }    
  }  


  // 11, 16 ROTATE LEFT / RIGHT, MOVE AWAY FROM WEST, CHANGE DIRECTION
  if ((calibration_step==11)|(calibration_step==16)){   

    if ((fabs(mx_horizontal - ERI(150+2*elevation_position))<20)){j=0;}
    j++;
    if ((j<11)){move_keep_running = 1;} // wait for 11 steps and move away from north
    else {

      while (move_pwm_state >0){}                       // wait for motor stop
      calibration_step += 1;
      move_direction = 5- move_direction;                // toggle left (2) right (3), change azimuth direction
      prev_move_direction = move_direction;
      move_plateau_repeat = move_plateau_repeat_[4];
      move_step = 0;
      move_pwm_state = 1;     
      j=0;
    }
  }

  // 12, 15 PASS WEST
  if ((calibration_step==12)|calibration_step==15){   
    if (calibration_step==12) {mx_horizontal_max = mx_horizontal; my_max = my_av; mx_horizontal_min = mx_horizontal; my_min = my_av;} 
    move_keep_running = 1;
    if ((mx_horizontal - ERI(150+2*elevation_position))*(2*move_direction-5) > 0)  // last term becomes -1 for left rotation !!West
    {
      calibration_step ++;
      prevmil_[2]=millis();       // store prevmil_[1] or prevmil_[2]                     
    }    
  }  

  // 13 MOVE AWAY FROM WEST OR SOUTH
  if ((calibration_step==13)){   
    move_keep_running = 1;
    prevmil_[1] = millis();
    if (fabs(mx_horizontal - ERI(150+2*elevation_position))>20){calibration_step++;}
  }

  // 14 APPROACHING WEST
  if ((calibration_step==14)){   
    move_keep_running = 1;
    mx = mx_av;
    mz = mz_av;    
    calc_mx_horizontal();    
    if ((fabs(mx_horizontal - ERI(150+2*elevation_position)) < 10) & (fabs(my_av - ERI(200+2*elevation_position)) < 50)){  //!!West
      calibration_step ++;
    }    
  }

  // 15 = 12, PASS WEST

  



  // 17 STORE RESULTS AND SET NEXT ELEVATION
  if ((calibration_step==17)){   

      
//     zenith_right_rotation_time = prevmil_[2] - prevmil_[1];


    if (elevation_position==(2*ERI(EA_el_steps))) EWI(EA_azimuth_rotation_msdeg, (prevmil_[2]-prevmil_[1])/360);                  //writing values to EEPROM
    EWI(250 + 2* elevation_position, (mx_horizontal_max+mx_horizontal_min)/2);      // mx_horizontal_neutral  Elevation_position = 0 for lowest elevation
    EWI(300 + 2* elevation_position, (mx_horizontal_max-mx_horizontal_min)/2);      // mx_horizontal_range
    EWI(350 + 2* elevation_position, (my_max+my_min)/2);                            // my_neutral
    EWI(400 + 2* elevation_position, (my_max-my_min)/2);                            // my_range

//      EWI(EA_mz_range, (ERI(EA_mx_range)+ERI(EA_my_range))/2);
    unsigned long prevmil_[5] = {0};                                                  //used to detect change      
    Serial.println();
    Serial.print(F("mxh<")); Serial.println(mx_horizontal_max);
    Serial.print(F("my<")); Serial.println(my_max);
    Serial.print(F("mxh>")); Serial.println(mx_horizontal_min);
    Serial.print(F("my>")); Serial.println(my_min);
    Serial.print(F("mxh_n=")); Serial.println(ERI(250 + 2* elevation_position));
    Serial.print(F("my_n=")); Serial.println(ERI(350 + 2* elevation_position));
    Serial.print(F("mxh_r=")); Serial.println(ERI(300 + 2* elevation_position));
    Serial.print(F("my_r=")); Serial.println(ERI(400 + 2* elevation_position));
    Serial.print(F("ms/deg=")); Serial.println(ERI(EA_azimuth_rotation_msdeg));
  
    if (elevation_position > 0){
      elevation_position -= 1;                                                                                        // last elevation_position is 0 corresponding to el_up_limit
      elevation_target = (ERI(EA_el_up_limit)*elevation_position + ERI(EA_el_down_limit)*((2*ERI(EA_el_steps))-elevation_position))/(2*ERI(EA_el_steps));
      calibration_step = 19;                                                                                          // same as 10, will then go to 18          
    }
    else {calibration_step = 20; print_compass = 0;}
  }

  // 18 START ROTATION IN OPPOSITE DIRECTION
  if ((calibration_step==18)){
      while (move_pwm_state >0){}                                                                                     // wait for motor stop
      move_direction = prev_move_direction;                                                                           // start rotating right
      move_plateau_repeat = move_plateau_repeat_[4];
      move_step = 0;
      move_pwm_state = 1;
      j=0;    
      calibration_step = 12;
  }

  // 20 CALIBRATION COMPLETE
  if (calibration_step==20){
      if (!(ERI(EA_calibrated)==1234)){                                                                               // store west direction corrections if this a primary calibration (not for recalibration)
        for (elevation1_position=0;elevation1_position<19;elevation1_position++){                                     // Presently these values are not used in the compass calculations. See comments in magnetometer()
          my = ERI(2*elevation1_position + 200);                                                                      // load my in West direction from EEPROM
          mx_horizontal = ERI(2*elevation1_position + 150);                                                           // load mx in West direction from EEPROM
          elevation_compass = float(ERI(EA_el_down_limit)) + ((float(ERI(EA_el_up_limit)-ERI(EA_el_down_limit))) / (2*ERI(EA_el_steps))) * float(elevation1_position);
          float temp_compass = (compass_calculation_2()-2700);
          if (temp_compass > 1800) temp_compass -= 3600;
          if (temp_compass < -1800) temp_compass += 3600;          
          EWI(750+2*elevation1_position,temp_compass);                                                                // store azimuth_angle in west direction to EEPROM, may be used in compass calculations (currently not used)
        }
      }
      
      EWI(EA_mx_neutral,temporary_mx_neutral);
      EWI(EA_mz_neutral,temporary_mz_neutral);
      EWI(EA_calibrated,1234);
      if (ERI(EA_incomplete_recalibration) == 1){
        calibration_step = 0;                                                                                         //restart normal operation after recalibration
                                                                                                                      //recalibration complete
      }
      else {
        calibration_step = 21;                                                                                        //wait for reboot
        Serial.println(F("complete --> restart"));
        // store west corrections here
        
      }

      EWI(EA_incomplete_recalibration,0);                                                                             // complete calibration
      EWI(EA_days_since_cal,0);                                                                                       // set days since last calibration to 0
      EWI(EA_azimuth_left_of_east,0);
      prev_azimuth_compass = 1900;
      
  }


  // 21 WAIT FOR REBOOT

  // 22 23 PREPARE TO START RECALIBRATION
  if ((calibration_step==22)|(calibration_step==23)){
    if (calibration_step==22){
      elevation_target = ERI(EA_el_up_limit);      
      azimuth_target = -900 + ERI(EA_north_corr);                        // recalibration starts in magnetic west direction with right movement
    }
    
    if ((micros()-last_micros_move > 1000000)&(move_pwm_state==0)){          // wait for 1s after last move

      if (calibration_step==22) {
        temporary_mx_neutral = ERI(EA_mx_neutral)-mx_raw;
        elevation_target = ERI(EA_el_down_limit);  // recalibration starts in lowest elevation position
      }
      else temporary_mz_neutral = ERI(EA_mz_neutral)-mz_raw;

      calibration_step +=1;                       // go to next calibration step and safe EEPROM parameters
      azimuth_target=10000;                       // stop azimuth tracking
      
    }
  }

/*
  // 24 PREPARE TO START RECALIBRATION
  if (calibration_step==24){
    elevation_target = ERI(EA_el_down_limit);      // recalibration starts in lowest elevation position
    azimuth_target = -900;                         // recalibration starts in west direction with right movement
    if ((micros()-last_micros_move > 1000000)&(move_pwm_state==0)){
      calibration_step +=1;                       // go to next calibration step and safe EEPROM parameters
      azimuth_target=10000;                       // stop azimuth tracking
    }
  }
*/


  // 24 MAKE SAFETY COPY OF EEPROM PARAMETERS AND START RECALIBRATION
  if (calibration_step==24){
    i= 0;
    while(i<150){
      EWI(2*i+450, ERI(2*i+150)); //copy EEPROM-positions 150..449 to 450..749. 2 bytes will store one int. 
      i++;
    }
    EWI(EA_incomplete_recalibration,1);           // set flag: incomplete recalibration, will restore safety copy upon restart
    i=30; 
    calibration_step = 9;
    mx_west = 0;
    my_west = 0;
    mz_west = 0;
    elevation_position = 0;
    serial_mode = 0;
  }
      


// STORM DETECTION 30, AND STOW 31
  if (calibration_step == 30){
    if (!(storm_flag)) calibration_step=0;
    else if ((millis()-prevmil_[3])/1000 > ERI(EA_wind_trigger_time)){  // wind gust above threshold is longer than wind_trigger_time
      calibration_step=31;
      azimuth_target = 10000;
      elevation_target = ERI(EA_el_target_stow);
    }
  }

  if (calibration_step == 31){
    if (storm_flag) prevmil_[3] = millis();
    else if ((millis()-prevmil_[3])/1000 > ERI(EA_wind_return_time)){  // wind is below threshold for longer than wind_return_time
      calibration_step = 0;
    }
  }
}
