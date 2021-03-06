void loop() {
  if ((calibration_step==0)&(ERI(EA_incomplete_recalibration) == 1)){ // incomplete calibration flag triggers restoring old calibration
    i= 0;
    while(i<150){
      EWI(2*i+150, ERI(2*i+450));   //copy EEPROM-positions  450..749 to 150..449. 2 bytes store one int. 
      i++;
    }
    calibration_step = 22;          // start recalibration skipping the user interaction in calibration steps 1..21
    last_micros_move = micros();
    Serial.println(F("copy restored"));
  }
  
  read_EEPROM_parameters();                               // copy EEPROM parameters used in timer interrupt to RAM

  handle_serial();                                        // user interface and outputs
  RTC_receive();                                          // get time from Real Time Clock

//  Serial.print(acceleration(0)); Serial.print(F("\t")); //
  acceleration();                                         // read accelerometer to calculate elevation
  
  magnetometer();                                         // get readings from magnetometer and perform compass calculations
  if (print_compass){/*RTC_print(); Serial.print(F("\t"));  Serial.print(millis()); */ Serial.print(F("\tElC=")); 
    Serial.print(elevation_compass); /*Serial.print(F(" ")); Serial.print(mx_raw);*/ Serial.print(F("\tmx="));  
    Serial.print(mx_raw); /*Serial.print(F("\t")); Serial.print(my_raw);*/ Serial.print(F("\tmy="));  Serial.print(my_raw); 
    /* Serial.print(F(" ")); Serial.print(mz_raw);*/ Serial.print(F("\tmz="));   Serial.print(mz_raw); Serial.print(F("\tmxh=")); 
    Serial.print(mx_horizontal); Serial.print(F(" "));
  }

  storm_flag = (wind_speed > ERI(EA_windspeed_stow));                                   // windspeed in clicks per second
//  if ((millis()>70000)&(millis()<90000)) {storm_flag = 1; Serial.println("storm");} //used to test recalibration after storm
  if (storm_flag&((calibration_step!=30)&(calibration_step!=31))) {calibration_step=30 ; prevmil_[3]=millis();}  //wait and check whether storm_flag remains high for EA_wind_trigger_time

  if ((calibration_step == 0)|(calibration_step==30)){   // 0: normal operation, 30: wind gust detected, operate and monitor wind
    if ((elevation_compass < ERI(EA_el_up_limit)) & (elevation_compass > ERI(EA_el_down_limit)) & (azimuth_compass < ERI(EA_az_right_limit)) & (azimuth_compass > ERI(EA_az_left_limit))){ track_sensor();}
    else sensor_track=0;
    // this initiates moves within the function based on sun sensor readings; Sensor tracking is suppressed for out of range astronomical values
    astro_track();                                       // no moves, only calculations of target values for Azimuth and elevation. Moves happen in goto_target() below
  }
  else sensor_track = 0;                                 // no sensor tracking during calibration other than calibration_step 3
  if ((calibration_step==3)&!Serial.available()) track_sensor();               // in calibration_step_3 the sun sensor needs to be adjusted to the sun, for this we need sun tracking
  if (ERI(EA_elevation_overcurrent)>7850){analogReference(DEFAULT);}                            // 5V ref. for current sensing, enable if you need to set a elevation current limit greater than 7.8A
  
  calibrate();                                           // swapped calibrate and goto_target, may have undesired results
  if (sensor_track == 0) {goto_target(); }               // goto_target() controls the motors based on the difference between compass and target position

  else if (!(azimuth_target==prev2_azimuth_target)){
    // increase counter, if new values give a worse fit than the old values because the astronomical tracking is running ahaed.
    astro_ahead_counter += -1 + 2*(fabs(azimuth_target-azimuth_compass)+fabs(elevation_target-elevation_compass) > fabs(prev2_azimuth_target-azimuth_compass)+fabs(prev2_elevation_target-elevation_compass));
//    Serial.print(F(" Astro_ahead: ")); Serial.print(astro_ahead_counter); Serial.print(F(" seconds: ")); Serial.println(BCD2DEC(i2c_read_byte(RTC_ADDRESS,0x00)));
    prev2_azimuth_target = azimuth_target;
    prev2_elevation_target = elevation_target;
    
    if((BCD2DEC(i2c_read_byte(RTC_ADDRESS,0x00))==30)&(ERI(EA_clock_corr_count)>0)){ //0 disables the counter
      if (astro_ahead_counter >= ERI(EA_clock_corr_count)) {i2c_write_byte(RTC_ADDRESS,0x00,DEC2BCD(28))  ; astro_ahead_counter = 0;}
      if (astro_ahead_counter <= -ERI(EA_clock_corr_count)){i2c_write_byte(RTC_ADDRESS,0x00,DEC2BCD(32))  ; astro_ahead_counter = 0;}
    }

  }


  //TURBO
  if (ERI(EA_turbo)){                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    // if the user has set the turbo flag in the EEPROM
    if(BCD2DEC(i2c_read_byte(RTC_ADDRESS,0x00))<59) i2c_write_byte(RTC_ADDRESS,0x00,DEC2BCD(59))  ;        //  always set the RTC seconds to 59 to speed up by factor ~50
  }
  if(print_compass != 0) Serial.println();               // print compass will create a Serial.println(), we need only one

//  Serial.println(); Serial.print(analogRead(A0)); Serial.print("\t"); Serial.print(analogRead(A1)); // 
  float elevation_current=analogRead(A0)*7.68049*(1+3.545*(ERI(EA_elevation_overcurrent)>7850));
//  Serial.println(elevation_current);
  if (elevation_current > ERI(EA_elevation_overcurrent)){   //7.68 mA per digit
    while(1){
      Serial.println(F("overcurrent"));
      handle_serial();
    }
  }
  analogReference(DEFAULT
  );
}
