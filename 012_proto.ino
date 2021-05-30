
bool MPU9250_setup();               // initiate 9 axis compass
void acceleration();                // get acceleration data
void magnetometer();                // get magnetometer data
void goto_target();                 // initiate moves in the direction of the target compass position
void calibrate();                   // this function handles the different calibration states and also 
                                    // some special modes such as the storm routine
void read_EEPROM_parameters();      // all parameters needed in interrupt routines are coppied to RAM
void debounceInterrupt();
byte handle_serial();
void RTC_receive();
void track_sensor();
void astro_track();
byte BCD2DEC(byte received_);
byte DEC2BCD(byte dec_);
void RTC_print();
void EEPROM_check_limits();
void i2c_start_read_burst(byte I2C_7BITADDR_, byte MEMLOC_);
void compass_calculations();
void calc_mx_horizontal();
void correct_azimuth_compass_range_to_0_3600();
float compass_calculation_2();

int ERI(int adr);                   //EEPROM read integer
void EWI(int adr, int wert);        //EEPROM write integer
bool i2c_write_byte(byte i2c_7BITADDR__, byte MEMLOC__, int byte2write__);  // write a single byte via I2C
byte i2c_read_byte(byte i2c_7BITADDR__, byte MEMLOC__);                     // read a single byte via I2C

// void(* resetFunc) (void) = 0; //declare reset function @ address 0, better use watchdog-timer if necessary
