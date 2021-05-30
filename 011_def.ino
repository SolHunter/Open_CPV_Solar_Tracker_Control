#define MPU9250_ADDRESS 0x69    // needed to change this from 0x68 by setting ADD high
#define MAG_ADDRESS 0x0C
#define RTC_ADDRESS 0x68
#define stormpin 3              // D3, interrupt 1, this must be an interrupt pin
#define debouncing_time 12000   // Debouncing Time in Microseconds, 12000 = 12ms allows a max. windspeed of 55 m/s = 200 km/h

#define PORTB_mask B11101000    // mask for port B (D13..D8) with all motor control pins set to 1, used to leave all other pins unaffected
#define PORTD_mask B00101011    // mask for port D (D7..D0) with all motor control pins set to 1
 

//DEFINE ALL EEPROM ADRESSES    // EA_ is for EEPROM address. Most parameters in EEPROM are not mirrored to RAM but called by ERI(EA_...)
                                // Parameters in EEPROM are represented by ASCII signs in the Serial Interface.

#define EA_turbo 2              // < initiate turbo mode (< is the associated ASCII sign)
#define EA_days_since_cal 6     // > days since calibration
#define EA_days_for_cal 8       // ? days for triggering recalibration
#define EA_clock_corr_count 10       // ? days for triggering recalibration
                                // all angles are given in steps of 0.1°, default and range is displayed in the comments for reference
#define EA_el_up_corr  46       // R 0 -100..100 correction for 90° Elevation measurement (Zenith)
#define EA_el_down_corr  48     //  S 0 -100..100 correction for 0° Elevation measurement (morning)

#define EA_north_corr  50       //  T North correction

#define EA_az_right_limit 52    //  U 3400  3400..3000
#define EA_az_left_limit  54    //  V -1500  -1200..-1600

#define EA_longitude  56        //  W       
#define EA_latitude 58          //  X   

#define EA_mx_neutral 60        //  Y approximat correction of the x magnetic axis. Locked. Determinded by automatic cal. routine.
#define EA_mz_neutral 62        //  Z approximat correction of the z magnetic axis. Locked. Determinded by automatic cal. routine.


#define EA_el_up_limit 64      // [ 700 900 Highest possible elevation position (will override move to higher positions).
#define EA_el_steps 66         // \ 1..9 double to get number of elevation steps. e.g. 9 will yield 18 steps and 19 positions
#define EA_el_down_limit 68    // ] 0..300  Lowest possible Elevation position (will override move to lower positions)
                               // 64,66,68 locked after calibration

#define EA_el_target_stow 78    //  b  850 0..900  elevation angle of stow position
#define EA_az_target_clean  80  //  c  1800  0..3600 Azimuth angle of cleaning position c
#define EA_el_target_clean  82  //  d  100 0..900  Elevation angle of cleaning position c
#define EA_az_target_home 84    //  e  1800  0..3600 Azimuth angle of home position h
#define EA_el_target_home 86    //  f  850 0..900  Elevation angle of home position h
#define EA_el_sleep 88          //  g  5 -50..200  Go to sleep and wake up Elevation angle (below this angle it is night)

#define EA_sensor_direct_sun  92            //  i 1/4096  400 10..1000  sun sensor threshold for sun sensor tracking
#define EA_sensor_step_error_azimuth  94    //  j 0.1%  50  10..500 sensor azimuth misbalance threshold for tracking step
#define EA_sensor_step_error_elevation  96  //  k 0.1%  50  10..500 sensor elevation misbalance threshold for tracking step
#define EA_sensor_run_error_azimuth 98      //  l 0.1%  100 10..1000  
#define EA_sensor_run_error_elevation 100   //  m 0.1%  100 10..1000  sensor misbalance threshold for fullspeed tracking
#define EA_sensor_hysteresis  102           //  n 0.1%  2000  1000..10000 sensor hysteresis that supresses reverse sensor <--> astro track
#define EA_sensor_direction_hysteresis 104  //  o 0.1%  2000  1000..10000 sensor hysteresis that supresses direction reversals
#define EA_astro_step_error_azimuth 106     //  p  3 1..200  
#define EA_astro_step_error_elevation 108   //  q  3 1..200  
#define EA_astro_run_error_azimuth  110     //  r  20  1..200  
#define EA_astro_run_error_elevation  112   //  s  20  1..200  

#define EA_calibrate_step_error_elevation 114 //  t  3 1..200   

#define EA_elevation_overcurrent 116        // the tracker will halt the programm if this current level is surpassed
#define EA_windspeed_stow 118               //  v   300 1..1023 wind speed threshold (clicks/10s) for go to stow
#define EA_wind_trigger_time  120           //  w s 10  1..600  length of wind gust to trigger storm
#define EA_wind_return_time 122             //  x s 600 1..3600 wait time for returning from storm
#define EA_azimuth_rotation_msdeg 124       //  y 600 1..3600 time for 1° azimuth rotation
#define EA_azimuth_left_of_east 126         //  z s 0 0|1 time for 1° azimuth rotation

#define EA_first_start    142               // 1234 if NOT first start, not user definable
#define EA_operation_days   144             // total operation days counter
#define EA_calibrated   146                 // set to 1 after calibration
#define EA_incomplete_recalibration   148   // set to 1 during recalibration, triggers restoring old calibration at next start
