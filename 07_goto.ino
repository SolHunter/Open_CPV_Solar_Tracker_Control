// COMPARE TARGET POSITION TO COMPASS POSITION AND INITIATE MOVE

void goto_target(){


  if (azimuth_target!=10000){                                                                 // 10000 is used to suppress moves

    if (azimuth_target < ERI(EA_az_left_limit)){azimuth_target = ERI(EA_az_left_limit);}      // prevent out of range targets, should never happen, but better be sure
    if (azimuth_target > ERI(EA_az_right_limit)){azimuth_target = ERI(EA_az_right_limit);}
    if (move_pwm_state == 0){                                                                 // always wait until previous move has ended
      move_direction = (azimuth_target > azimuth_compass) + 2;                                // calculate move_direction azimuth left or right
      if (abs(azimuth_target-azimuth_compass)>ERI(EA_astro_run_error_azimuth)){               // check condition for fullspeed correction
        move_plateau_repeat = move_plateau_repeat_[4];                                        // fullspeed plateau duty cycle azimuth
        move_step = 0;                                                                        // fullspeed movement
        move_pwm_state = 1;                                                                   // command for starting movement in the ISR(TIMER1_OVF_vect)
      }
      else if (abs(azimuth_target-azimuth_compass)>ERI(EA_astro_step_error_azimuth)){         // check condition for step correction
        move_plateau_repeat = move_plateau_repeat_[move_direction];                           // step plateau duty cycle azimuth
        move_step = 1;                                                                        // move in steps, don't reset plateau counter
        move_pwm_state = 1;                                                                   // command for starting movement in the ISR(TIMER1_OVF_vect)
      }
    }
    else if ((move_pwm_state == 3) || (move_pwm_state == 4)){                                 // check for plateau
      if(((azimuth_target-azimuth_compass> ERI(EA_astro_run_error_azimuth) )&&(move_direction==3)) || ((azimuth_target-azimuth_compass<-ERI(EA_astro_run_error_azimuth))&&(move_direction==2))   ){
        move_keep_running = 1;                                                                // keep running if the error is still large enough in the right direction
      }                       
    }    
  }


  if (elevation_target!=10000){
    if (elevation_target < ERI(EA_el_down_limit)){elevation_target = ERI(EA_el_down_limit);}  // correct to allowed limits, never move below lower eleveation limit in astronomical mode
    if (elevation_target > ERI(EA_el_up_limit)){elevation_target = ERI(EA_el_up_limit);}
    if (move_pwm_state == 0){                                                                 // Only start if motors are idle, azimuth corrections are performed before elevation corrections.
      move_direction = (elevation_target < elevation_compass);

      if (abs(elevation_target-elevation_compass)>ERI(EA_astro_run_error_elevation)){
        move_plateau_repeat = move_plateau_repeat_[4];
        move_step = 0;
        move_pwm_state = 1;
      }
      else if (abs(elevation_target-elevation_compass) > ERI(EA_astro_step_error_elevation)*((calibration_step==0)|(calibration_step==255)) + ERI(EA_calibrate_step_error_elevation)*!((calibration_step==0)|(calibration_step==255))){
        move_plateau_repeat = move_plateau_repeat_[move_direction];
        move_step = 1;
        move_pwm_state = 1;
      }
    }
    else if ((move_pwm_state == 3) || (move_pwm_state == 4)){ 
      if(((elevation_target-elevation_compass>ERI(EA_astro_run_error_elevation))&&(move_direction==0)) || ((elevation_target-elevation_compass<-ERI(EA_astro_run_error_elevation))&&(move_direction==1))   ){
        move_keep_running = 1;
      }
    }
    
  }
}
