void astro_track(){
  // Saheli  www.ijetae.com (ISSN 2250-2459, Volume 2, Issue 9, September 2012)
  // float B = 2*PI()/364*(days_in_year-81);
  // float eqtime = (0.165*SIN(2*B)-0.126*COS(B)-0.025*SIN(B)) *60;
  // float declination = 23.45*PI()/180*SIN(2*PI()*(284+days_in_year)/365)
  
  frac_year *= 2*PI; //!!Saheli delete and replace by days_in_year

  // approximation of the time offset solar time vs clock time which changes during the year
  float eqtime = 229.18*(0.000075+0.00186*cos(frac_year)-0.032077*sin(frac_year)-0.01461*cos(2*frac_year)-0.040849*sin(2*frac_year));
  // approximation of the declination change during the year
  float declination = 0.006918-0.399912*cos(frac_year)+0.070257*sin(frac_year)-0.006758*cos(2*frac_year)+0.000907*sin(2*frac_year)-0.002697*cos(3*frac_year)+0.00148*sin(3*frac_year);
  // we use the UTC time at longitude 0. Therefore we need to correct by 4 minutes for each degree of longitude. The longitude is given in steps of 0.1° --> divide by 10
  //               time_offset = eqtime + 4*ERI(EA_longitude)/10;

  hour_angle = 2*PI*(frac_day + (eqtime + 4* ERI(EA_longitude)/10)/(60*24) - 0.5);          // hour angle is 0 at noon and jumps from Pi to -Pi at midnight
  if (hour_angle > PI){hour_angle -= 2*PI;}                                                 // move hour angle into allowed range [-Pi..Pi] for calculations
  if (hour_angle < -PI){hour_angle += 2*PI;}

  // Here some trigonometry happens in spherical coordinates
  float temp1 = (sin((PI/1800)*ERI(EA_latitude))*sin(declination)+cos((PI/1800)*ERI(EA_latitude))*cos(declination)*cos(hour_angle));
  elevation_target = 900-(1800/PI) * acos(temp1);                                           // convert to the default format
  azimuth_target = (1800 / PI) * acos(-(sin((PI/1800)*(ERI(EA_latitude)))*temp1-sin(declination))/(cos((PI/1800)*(ERI(EA_latitude)))*sqrt(1-temp1*temp1))) ;

  if (hour_angle > 0){azimuth_target = 3600 - azimuth_target;}                              
  // temp1 is symmetrical to noon. Azimuth target of course should not be symmetrical but proceeding. So we need to make this destinction by hour angle.
  // We also go back to the usual format in angular steps of 0.1° 

  //!!! This might be the best place to calculate a correction based on the last value "on sun" when sensor_track==2
  //!!! recover this correction when sensor_track==0
  
  if(print_compass){
    Serial.print(F(" el_t=")); Serial.print(elevation_target); 
    Serial.print(F(" az_t=")); Serial.print(azimuth_target); 
  }
  
  
  // CHECK FOR NIGHT
  if (elevation_target < ERI(EA_el_sleep)){
    if ((calibration_step ==0) & ERI(EA_days_since_cal)>ERI(EA_days_for_cal)) {calibration_step = 22; last_micros_move = micros();}       //start recalibration at night after days_for_cal. 
    azimuth_target = ERI(EA_az_target_home);
    elevation_target = ERI(EA_el_target_home);
    night = 1;
  }
  else {night = 0;}

//  Serial.print(F("\t el_t=")); Serial.print(elevation_target); 
//  Serial.print(F("\t az_t=")); Serial.print(azimuth_target); Serial.println();

  // CHECK IN RANGE
  if (elevation_target < ERI(EA_el_down_limit)){elevation_target = ERI(EA_el_down_limit);}
  if (elevation_target > ERI(EA_el_up_limit)){elevation_target = ERI(EA_el_up_limit);}  

  if (prev_azimuth_target == 10000){prev_azimuth_target = azimuth_target;}    // first pass of routine
  if (azimuth_target - prev_azimuth_target > 1801){azimuth_target -= 3600;}  // supress jumps
  if (azimuth_target - prev_azimuth_target < - 1801){azimuth_target += 3600;}  
  if (azimuth_target > ERI(EA_az_right_limit)){azimuth_target -= 3600;}        // turn 360° if necessary to stay in range
  if (azimuth_target < ERI(EA_az_left_limit)){azimuth_target += 3600;} 
  prev_azimuth_target = azimuth_target;                                       
  
  if (print_compass){
    Serial.print(F(" el_t=")); Serial.print(elevation_target); 
    Serial.print(F(" az_t=")); Serial.print(azimuth_target); 
  }
}
