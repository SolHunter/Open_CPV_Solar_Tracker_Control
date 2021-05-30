#define SDA_PORT PORTB                // Definitions need to be placed before #include <SoftI2CMaster.h>
#define SDA_PIN 3 // = D11
#define SCL_PORT PORTD
#define SCL_PIN 5 // = D5
#define i2c_TIMEOUT 100
#include <SoftI2CMaster.h>            // Soft I2C by Felias Fogg allows using arbitrary pins since I2C pins are already in use by the dual VNH5019 shield.

#include <EEPROM.h>                   // for storing parameters and calibration results
#include <math.h>                     // for astronomical calculations


// ***************************
// Movements and Steps                // A custom PWM is controlled by a timer interrupt routine. 
//****************************        // The following variables interact with this routine.


volatile byte move_direction = 3;     // 0: up, 1: down, 2: left, 3: right
byte prev_move_direction;             // used to store last rotation direction, e.g. left or right
volatile byte move_speed_now = 1;     // present move speed, possible values 1..16
volatile byte move_speed_[8];         // plateau speed for each move_direction, [4-7] for steps, possible values 0..16
volatile byte move_step = 1;          // one if stepping, 0 for continuous. Allows different final speeds for steps
volatile byte move_ramp_repeat_[4];   // how often should a speed value be repeated before increment or decrement, high value --> slow ramp
int move_plateau_repeat_[5];          // How often should the plateau state be repeated for tracking steps for different move_direction
int move_plateau_repeat=100;          // How often should the plateau be repeated
volatile byte move_keep_running = 0;  //resets move_repeated to 0 when "1" such that the plateau continues, "2" initiates immediate stop
int move_brake_repeat;                // how often should the brake cycle be repeated before stopping
volatile int move_repeated = 0;       // counter for speed value repetitions, incremented in ISR
volatile byte move_pwm_state = 0;     // 0: stop, 1: ramp up on, 2: ramp up off, 3: plateau on, 4: plateau off, 5: ramp down on, 6: ramp down off, 7:break
                                      // start motor by setting 1, initiate stop by setting 5 or 6 (or just wait), will continue to 0 after 7, ready for new direction

                                      // pre-define states for digital outputs on PORTB (and PORTD) in different motor switching states. This allows to change hardware easily.
                                      // arrays allow small code. E.g. PORTB = move_portb_[3*move_direction] will turn motor on actively in the respective move direction
byte move_portb_[9];                  // predefined bytes to set PORTB and motor-controler states on pins D8..D13
byte move_portd_[9];                  // predefined bytes to set PORTD and motor-controler states on pins D4..D7 (the 4 least significant bits must be 0)
byte motorlocks=0;

// ***************************
// Windspeed calculation
// ***************************

volatile unsigned long last_micros = 0;
volatile int wind_speed = 0;                                  // wind speed in clicks per 10 seconds

// ***************************
// compass calculations
// ***************************

float mx, mx_[10], mx_raw, mx_horizontal_max, mx_horizontal_min, mx_av, mx_av_[10], my, my_[10], my_raw, my_max, my_min, my_av, my_av_[10], mz, mz_[10], mz_raw, mz_max, mz_min, mz_av, mz_av_[10], azimuth_compass, elevation_compass, temperature, mx_horizontal;
float prev_azimuth_compass; 
float averaged_azimuth_compass = 10000;                       // 10000 marks invalid value after restart
byte first_pass = 1;                                          // workaround for wrong Azimuth readings after upload of code

// ***************************
// astronomical tracking calculations
// ***************************

float azimuth_target = 10000; 
float elevation_target=10000;   //move tracker to these values, 10000 disables the movement
float prev_azimuth_target = 10000;
float prev2_azimuth_target, prev2_elevation_target;
float received, frac_year, frac_day, hour_angle;              // generated by RTC
int astro_ahead_counter;

// ***************************
// Calibration routine and special modes
// ***************************

byte i=0; byte j=0; byte m, k;
byte calibration_step=0;                                      // pointer on the present calibration step. 0 for normal operation
byte prev_calibration_step=0;                                 // required for serial output of new calibration step

byte elevation_position, elevation1_position;                 // elevation position can have values 0..(2*ERI(EA_el_steps))
float elevation1_position_float;        // needed for interpolation between discrete values in elevation
byte temp_byte;                         // used in calibration

byte print_compass = 0;                 // toggle between printing compass and not printing compass with a serial "%" entry
float mx_west, my_west, mz_west;        // used for averaging west readings before storing value to EEPROM
// float mx_north, my_north, mz_north;  // used for oversampling in calibrate

int temporary_mx_neutral, temporary_mz_neutral;

unsigned long prevmil_[4];              // zenith_right_rotation_time; 

byte serial_mode = 0;                   // required to distinguish between different functionalities of the serial interface, also used to return entered values.

bool storm_flag;                        // used in the storm routine
byte prev_hour = 0;                     // detect change of day to count days since last calibration
unsigned long last_micros_move;

// ***************************
// Sensor Tracking
// ***************************

int sens_total, sens_up, sens_right;    // sensor readings, individual sensor channels are local variables
byte sensor_track = 0;                  // disables sensor tracking for low sun or calibration steps
byte night;
byte prev_elevation_direction, prev_azimuth_direction;  // suppress direction reversals in sensor tracking mode




const int EEPROM_limits[] PROGMEM =      // pre defined limits for EEPROM resident values in 
{                                        // program memory space (flash) to safe SRAM space
                                         // EEPROM may be restored to presets with serial command #
/*0 ; check 1234 if calibrated        */
-32000, /*lower limit*/
32000, /*upper limit*/
111, /*preset*/

/*1 < EA_turbo speed up time by a factor of 50 for testing */
0, /*lower limit*/
1, /*upper limit*/
0, /*preset*/

/*2 =         */
111, /*lower limit*/
111, /*upper limit*/
111, /*preset*/

/*3 > days_since_cal        */
0, /*lower limit*/
1000, /*upper limit*/
0, /*preset*/

/*4 ? days_for_cal          */
1, /*lower limit*/
1000, /*upper limit*/
30, /*preset*/

/*5 @ EA_clock_corr_count */
0, /*lower limit*/
32767, /*upper limit*/
3600, /*preset, approx. 1 hr.*/

  
/*6 A         */
111, /*lower limit*/
111, /*upper limit*/
111, /*preset*/

/*7 B move_brake_repeat         */
1, /*lower limit*/
10000, /*upper limit*/
500, /*preset*/

/*8 C move_speed_[0] up        */
1, /*lower limit*/
16, /*upper limit*/
8, /*preset*/

/*9 D move_speed_[1] down         */
1, /*lower limit*/
16, /*upper limit*/
8, /*preset*/

/*10 E move_speed_[2 3] left right           */
1, /*lower limit*/
16, /*upper limit*/
8, /*preset*/

/*11 F move_speed_[4] step up       */
1, /*lower limit*/
16, /*upper limit*/
8, /*preset*/

/*12 G move_speed_[5] step down         */
1, /*lower limit*/
16, /*upper limit*/
8, /*preset*/

/*13 H move_speed_[6 7] step left right        */
1, /*lower limit*/
16, /*upper limit*/
8, /*preset*/

/*14 I move_ramp_repeat_[0] up        */
1, /*lower limit*/
255, /*upper limit*/
10, /*preset*/

/*15 J move_ramp_repeat_[1] down       */
1, /*lower limit*/
255, /*upper limit*/
10, /*preset*/

/*16 K move_ramp_repeat_[2 3] left right       */
5, /*lower limit*/
255, /*upper limit*/
10, /*preset*/

/*17 L move_plateau_repeat_[0] step up, 0.5ms per repetition        */
1, /*lower limit*/
5000, /*upper limit*/
10, /*preset*/

/*18 M  move_plateau_repeat_[1] step down       */
1, /*lower limit*/
5000, /*upper limit*/
10, /*preset*/

/*19 N  move_plateau_repeat_[2 3] left right */
1, /*lower limit*/
5000, /*upper limit*/
10, /*preset*/

/*20 O  move_plateau_repeat_[4] continuous move       */
1000, /*lower limit*/
5000, /*upper limit*/
1500, /*preset*/

/*21 P unused        */
111, /*lower limit*/
111, /*upper limit*/
111, /*preset*/

/*22 Q unused         */
111, /*lower limit*/
111, /*upper limit*/
111, /*preset*/

/*23 R  elevation_up_correct        */
-100, /*lower limit*/
100, /*upper limit*/
-100, /*preset*/                            // start with 10° negative correction to avoid crash

/*24 S elevation_down_correct        */
-100, /*lower limit*/
100, /*upper limit*/
100, /*preset*/

/*25 T EA_north_corr 52        */
-500, /*lower limit*/
500, /*upper limit*/
0, /*preset*/

/*26 U EA_az_right_limit 52       */
3000, /*lower limit*/
3400, /*upper limit*/
3400, /*preset*/

/*27 V EA_az_left_limit         */
-1600, /*lower limit*/
-1200, /*upper limit*/
-1600, /*preset*/

/*28 W EA_longitude        */
0, /*lower limit*/
3599, /*upper limit*/
0, /*preset*/

/*29 X EA_latitude 60        */
-800, /*lower limit*/
800, /*upper limit*/
0, /*preset*/

/*30 Y EA_mx_neutral    */
-32000, /*lower limit*/
32000, /*upper limit*/
0, /*preset*/

/*31 Z EA_mz_neutral    */
-32000, /*lower limit*/
32000, /*upper limit*/
0, /*preset*/

/*32 EA_el_up_limit */
700, /*lower limit*/
900, /*upper limit*/
900, /*preset*/

/*33 \ EA_el_steps half number of elevation steps         */
1, /*lower limit*/
12, /*upper limit*/
1, /*preset*/

/*34 ] EA_el_down_limit         */
0, /*lower limit*/
300, /*upper limit*/
150, /*preset*/

/*35 ^         */
111, /*lower limit*/
111, /*upper limit*/
111, /*preset*/

/*36 _         */
111, /*lower limit*/
111, /*upper limit*/
111, /*preset*/

/*37 '        */
111, /*lower limit*/
111, /*upper limit*/
111, /*preset*/

/*38 a         */
111, /*lower limit*/
111, /*upper limit*/
111, /*preset*/

/*39 b EA_el_target_stow        */
0, /*lower limit*/
900, /*upper limit*/
850, /*preset*/

/*40 c EA_az_target_clean */
-900, /*lower limit*/
2700, /*upper limit*/
900, /*preset*/

/*41 d EA_el_target_clean  */
0, /*lower limit*/
900, /*upper limit*/
450, /*preset*/

/*42 e EA_az_target_home  */
-900, /*lower limit*/
2700, /*upper limit*/
900, /*preset*/

/*43 f EA_el_target_home        */
0, /*lower limit*/
900, /*upper limit*/
850, /*preset*/

/*44 g EA_el_sleep        */
-50, /*lower limit*/
200, /*upper limit*/
50, /*preset*/

/*45         */
111, /*lower limit*/
111, /*upper limit*/
111, /*preset*/

/*46 i EA_sensor_direct_sun        */
10, /*lower limit*/
20000, /*upper limit*/
400, /*preset*/

/*47 j EA_sensor_step_error_azimuth        */
10, /*lower limit*/
500, /*upper limit*/
50, /*preset*/

/*48 k EA_sensor_step_error_elevation        */
10, /*lower limit*/
500, /*upper limit*/
50, /*preset*/

/*49 l EA_sensor_run_error_azimuth        */
10, /*lower limit*/
1000, /*upper limit*/
100, /*preset*/

/*50 m EA_sensor_run_error_elevation        */
10, /*lower limit*/
1000, /*upper limit*/
100, /*preset*/

/*51 n EA_sensor_hysteresis        */
0, /*lower limit*/
800, /*upper limit*/
500, /*preset*/

/*52 o EA_sensor_direction_hysteresis   */
0, /*lower limit*/
800, /*upper limit*/
500, /*preset*/

/*53 p EA_astro_step_error_azimuth        */
1, /*lower limit*/
200, /*upper limit*/
3, /*preset*/

/*54 q EA_astro_step_error_elevation        */
1, /*lower limit*/
200, /*upper limit*/
3, /*preset*/

/*55 r EA_astro_run_error_azimuth        */
1, /*lower limit*/
200, /*upper limit*/
20, /*preset*/

/*56 s EA_astro_run_error_elevation        */
1, /*lower limit*/
200, /*upper limit*/
20, /*preset*/

/*57 t EA_calibrate_step_error_elevation */
1, /*lower limit*/
20, /*upper limit*/
3, /*preset*/   

/*58 u EA_elevation_overcurrent */
0, /*lower limit*/
32767, /*upper limit*/
16000, /*preset corresponds to 1A*/

/*59 v EA_windspeed_stow in clicks per 10s*/
-1, /*lower limit*/   // -1 will always enable storm (for testing)
1024, /*upper limit*/ //1024 will disable storm function  
300, /*preset for 20 meters per second and N25FR sensor*/

/*60 w EA_wind_trigger_time */
1, /*lower limit*/
100, /*upper limit*/
10, /*preset*/

/*61 x EA_wind_return_time */
1, /*lower limit*/
100, /*upper limit*/
10, /*preset*/

/*62 y EA_azimuth_rotation_msdeg */
1, /*lower limit*/
32000, /*upper limit*/
1000, /*preset*/

/*63 z EA_azimuth_left_of_east        */
0, /*lower limit*/
1, /*upper limit*/
0, /*preset*/

};
