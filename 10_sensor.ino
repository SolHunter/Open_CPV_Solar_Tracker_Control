void track_sensor(){
  if (night) return;
  int sens_[4];
  sens_total = 0;
  for (byte i_s = 0; i_s<4; i_s++){                                           //A0 and A1 are used for VNH5019 current sensing
//    sens_[i_s] = 1023 - analogRead(i_s+2);                                    //This is for a 4 quadrant photodiode sensor with the Anodes connected to 5V. Need to adapt for photoresistors.
    sens_[i_s] = analogRead(i_s+2);                                           // This is for a LED sun sensor
    //Serial.print(F(" ")); Serial.print(i_s); Serial.print(sens_[i_s]);
    sens_total += sens_[i_s];
  }
// Normal Orientation of sensor, for 0° rotated sensor, add - signs
  sens_up = -1023*float(- sens_[0] + sens_[1] + sens_[2] - sens_[3]) / sens_total;
  sens_right = 1023*float(- sens_[0] - sens_[1] + sens_[2] + sens_[3]) / sens_total;  

  
//  Serial.print(F(" st=")); Serial.print(sens_total);
//  Serial.print(F(" su=")); Serial.print(sens_up);
//  Serial.print(F(" sr=")); Serial.println(sens_right); 
//  Serial.print(sensor_track); Serial.print(" ");
//  Serial.println((sens_total*(1.0+sensor_track*float(ERI(EA_sensor_hysteresis)/100.0))));
  if (((sens_total*(1.0+sensor_track*float(ERI(EA_sensor_hysteresis)/100.0))) > ERI(EA_sensor_direct_sun))){   //!!! need to introduce bool(sensor_track) for sensor_track==2

    Serial.print(F(" sensT="));Serial.print(sens_total);Serial.print(F("\tU="));Serial.print(sens_up);Serial.print(F("\tR="));Serial.print(sens_right);
    Serial.print(F("\t1="));Serial.print(sens_[0]);Serial.print(F("\t2="));Serial.print(sens_[1]);Serial.print(F("\t3="));Serial.print(sens_[2]);Serial.print(F("\t4="));Serial.println(sens_[3]);    
    sensor_track=1; // use hysteresis, no constant swapping between modes
  
    if ((move_pwm_state == 0) & (sensor_track)){
      move_direction = (sens_right > 0) + 2;
//      Serial.println("here");
      if (abs(sens_right) > ERI(EA_sensor_run_error_azimuth)){
        move_plateau_repeat = move_plateau_repeat_[4];
        move_step = 0;
        move_pwm_state = 1;
      }
      else if ((abs(sens_up)<=ERI(EA_sensor_run_error_elevation))&(abs(sens_right)*(1.0+float(prev_elevation_direction == move_direction)*float(ERI(EA_sensor_direction_hysteresis))/100.0) > ERI(EA_sensor_step_error_azimuth))){
        move_plateau_repeat = move_plateau_repeat_[move_direction];
        move_step = 1;
        move_pwm_state = 1;
        prev_elevation_direction = move_direction;
      }
    }
    else if ((move_pwm_state == 3) || (move_pwm_state == 4)){ 
      if(((sens_right> ERI(EA_sensor_run_error_azimuth) )&&(move_direction==3)) || ((sens_right<-ERI(EA_sensor_run_error_azimuth))&&(move_direction==2))   ){
        move_keep_running = 1;
      }
    }    
  




//    if (elevation_target < ERI(EA_el_down_limit)){elevation_target = ERI(EA_el_down_limit);}
//    if (elevation_target > ERI(EA_el_up_limit)){elevation_target = ERI(EA_el_up_limit);}
    if (move_pwm_state == 0){
      move_direction = (sens_up <0);  // 0:up, 1: down
//      Serial.println("here");
      if (abs(sens_up)>ERI(EA_sensor_run_error_elevation)){
        move_plateau_repeat = move_plateau_repeat_[4];
        move_step = 0;
        move_pwm_state = 1;
      }
      else if (abs(sens_up)*(1.0+float(prev_azimuth_direction == move_direction)*float(ERI(EA_sensor_direction_hysteresis))/100)>ERI(EA_sensor_step_error_elevation)){
        move_plateau_repeat = move_plateau_repeat_[move_direction];
        move_step = 1;
        move_pwm_state = 1;
        prev_azimuth_direction = move_direction;
      }
    }
    else if ((move_pwm_state == 3) || (move_pwm_state == 4)){ 
      if(((sens_up > ERI(EA_sensor_run_error_elevation))&&(move_direction==0)) || ((sens_up<-ERI(EA_sensor_run_error_elevation))&&(move_direction==1))   ){
        move_keep_running = 1;
      }
    }
    // !!! maybe introduce sensor_track = 2 when sensor is on sun  if( (abs(sens_up)>ERI(EA_sensor_step_error_elevation)) | (abs(sens_up)>ERI(EA_sensor_step_error_elevation)) )  sensor_track = 2; )
  }
  else sensor_track = 0;
}  
