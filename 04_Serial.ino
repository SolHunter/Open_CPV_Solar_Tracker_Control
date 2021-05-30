/*
 * This is the routine which handles all the comunication through the serial port. It allow inputs to the EEPROM, outputs from the EEPROM as well as special commands.
The parameter serial_mode allows for different functionalities within the routine handle_serial(). Possible values
0: normal operation
1: waiting for single letter input
    64<serial_mode<123: ASCII code of entered character, e.g. 
    121 for y, 110 n, 117 u, 85 U, 100 d, 68D, 108 l, 76 L, 114 r, 82 R, 102 f
2: write single serial character to serial_mode as an integer number
4: write whole line from serial to 8 EEPROM integers 



 */




byte handle_serial() {
 byte relative_change = 1;             // relative changes to the EEPROM are the default. Set 0 for absolute writing to EEPROM
 byte negate = 0;                      // 1 if a - sign is detected in the string
 int serial_integer = 0;               // integer received through serial port
 char incoming_character;              // single incoming character through serial port
 char option_character;                // The incoming ASCII sign $ is for commands followed by the incoming sign option character
 byte EEPROM_pointer = 255;            // EEPROM pointer for writing single user defined parameters. corresponds to 58 ASCII letters, 
                                       // A-->0, z -->57, no letter received --> 255
 byte EEPROM_writepos = 255;           // EEPROM writeposition to restore a whole line to the EEPROM or writing a time & date to the RTC
 if (serial_mode>3){serial_mode=0;}    // serial modes >3 are used to return information from Serial. Reset to 0 after one main loop
 while ((Serial.available() > 0)&&(EEPROM_pointer == 255)) {    // don't read a character from Serial Buffer 
                                                                // if there is data to be written to EEPROM
   incoming_character = Serial.read(); // store one byte from serial buffer for later usage
   delay(1);                           // 1 ms delay to make sure that new incoming_character is available

  // READ SINGLE LETTER COMMAND UPON REQUEST serial_mode == 1
  if ((serial_mode==2)&&((int)incoming_character > 64)&&((int)incoming_character < 123)){    
                                                 // if the user has entered an ASCII sign as a "reply" to a request
    serial_mode = (int)incoming_character;       // store answer to request in serial_mode
    incoming_character = 0;                      // to avoid triggering other functions with the incoming character
  }

// # RESTORE FACTORY DEFAULTS 
   if (incoming_character == '#'){                                                     // restore factory defaults
     while ((Serial.available() > 0)) {Serial.read();}                                 // clear buffer
     Serial.println(F("restore defaults y/n"));
     while ((Serial.available() ==0)) {}                                               // wait for serial
     if (Serial.read()=='y'){
      while ((Serial.available() > 0)) {Serial.read();}                                // clear buffer
        for (byte hs_counter = 0; hs_counter < 58; hs_counter++){
          EWI(int(hs_counter*2), int(pgm_read_word(&EEPROM_limits[hs_counter*3+2])) ); 
        }                                           // get default values from program memory and write them to EEPROM
      Serial.println(F("restored"));
     }
   }

// SOME SPECIAL COMMANDS
   if (incoming_character == '$'){                  // some two letter commands &+number, number space for 5 more commands available e.g. $3
    option_character = Serial.read();
    if (option_character=='0') calibration_step=0;  // return to normal tracking
    if (option_character=='1') {                    // go to cleaning position
      calibration_step=255; 
      sensor_track=0; 
      elevation_target = ERI(EA_el_target_clean);   // EA_el_target_clean contains the elevatio position in percent of the full range    

      azimuth_target = ERI(EA_az_target_clean);}
    if (option_character=='|') {                      // manual movements and sensor tracking without astro.
      calibration_step=3;                             // Carefull! This jumps into the calibration routine! Exit with $0
      azimuth_target=10000; elevation_target==10000;  // disable moves in go_to_target   
      Serial.print(F("\nCaution! Manual Moves \nc UDLR s, $0 return\n\n"));}  
    if (option_character=='8') {calibration_step=22; last_micros_move = micros();}      //initiate recalibration        
    if (option_character=='9') {calibration_step=1; EWI(EA_calibrated,0);}        //initiate new calibration    
   }

// / RETURN FINISH INFORMATION
   if (incoming_character == '/'){                     // finish command, used in calibration_step == 2
     serial_mode=int('/');                                                                        
   }   

// # TOGGLE COMPASS OUTPUT
   if (incoming_character == '%'){                                           // restore factory defaults
     print_compass = !print_compass;
   }

// ~ OUTPUT EEPROM CONTENT
   if (incoming_character == '~'){                                           // output EEPROM content
     if (Serial.read()=='1'){
       for (int i_=0 ; i_<1024 ; i_=i_+2) {
         Serial.print(ERI(i_)); Serial.println(F(","));
       } // output complete EEPROM as a list of 512 signed integer numbers
     }
     else {
       for (byte hs_counter = 0; hs_counter < 64; hs_counter++){
         if ((hs_counter % 8)==0){                                           // each line shows 8 values, start new line after 0,8,16, etc. values
          Serial.println(F("\t!"));Serial.print(F("*!")); Serial.print(hs_counter/8); // new line after 8 values start with *!
         }
         Serial.print(F("\t")); Serial.print(char(hs_counter + 59)); Serial.print(ERI(int(hs_counter*2)));  
       }                                                        //output ACII character followed by the associated parameter value from EEPROM
  
       // OUTPUT RTC TIME
       Serial.println(F("\t!"));Serial.print(F("*!")); Serial.print(F("8")); // the last line of the output is the RTC time
       RTC_print();                                                                                       
       Serial.println(F("\t!"));    // end of last line                      // end characters for the last line in the block
     } 
   }



// *! WRITE LINE TO EEPROM TO MIRROR A STORED CONFIGURATION
   if ((incoming_character == '*') && (Serial.read()=='!')){    //check for *! to write to EEPROM line 
     serial_mode = 4;                                           //flag for writing line to EEPROM
     EEPROM_writepos = (int(Serial.read())-48)*8;               //read start position for EEPROM write
     Serial.read();                                             //discard the next sign through serial
   }                                                            //finish and get one more incoming_character from serial buffer, 
                                                                //serial_mode==4 suppresses all undesired actions in the loop
   if ((serial_mode==4)&&(incoming_character == '!')){
     serial_mode = 0;                                           // finished writing a line to EEPROMs, exit from serial_mode==4
     EEPROM_check_limits();                                     // get parameters in EEPROM back to allowed limits
   }                                                      

   if ((incoming_character == '\t')&&(serial_mode==4)){         // tabstop marks beginning of new value
     if(int(EEPROM_writepos)<64){                               // select writing to EEPROM for first 64 write positions
       EWI(int(EEPROM_writepos*2), serial_integer*(1-negate));  // write serial_integer to EEPROM. negate==2 will result in a negative value
       EEPROM_writepos = int(EEPROM_writepos)+1;                
       serial_integer = 0;                                      // ready to assemble a new integer value from a serial string
       negate = 0;                                              // preset to positive integer
     }
     else{                                                      // writing to RTC
       if (EEPROM_writepos == 67) EEPROM_writepos ++;           // skip day of week
       i2c_write_byte(RTC_ADDRESS,6+64-EEPROM_writepos,DEC2BCD(byte(serial_integer)));   
       EEPROM_writepos ++;                                      // writing to RTC, calculate registers 0..7 from increasing EEPROM writepos
       serial_integer = 0;
     }
   }


// -0123 READ AN INTEGER VALUE OF SEVERAL DIGITS // reading the integer will finish when incoming_character is an ASCII sign that stands 
                                                 // for an EEPROM position or when reading serial_mode=4 and incoming character = "\t"
   if (serial_mode<5){                           // Serial mode also acts as a variable for returning values to other functions with serial_mode>5
     if (((int) incoming_character > 47) && ((int) incoming_character <58)){     // if incoming character is a single letter number
       serial_integer = 10* serial_integer + (incoming_character-48);            // assemble a decimal integer number
     }
     
     if (incoming_character == '-') {negate = 2;}                                // 2: negate, 0: don't negate
     if (incoming_character == ':') {relative_change=0;}                         // write a relative value to EEPROM (default: absolute value)
     
     if (((int)incoming_character > 58)&&((int)incoming_character < 123)&&(int(EEPROM_writepos) == 255)) {
       EEPROM_pointer = incoming_character - 59;                                 // received an ASCII sign that stands for an EEPROM position
     }    
   }
   
   if (serial_mode==1){serial_mode=0;}                                           
 }

// PRINT THE LAST EEPROM WRITE POSITION AFTER RESTORING A FULL LINE TO EEPROM
 if (EEPROM_writepos<255){Serial.println(int(EEPROM_writepos));}                 // EEPROM_writepos == 255 stands for no EEPROM write access in this loop


// CORRECT THE ENTERED VALUE BY HARD CODED LIMITS, WRITE TO EEPROM AND PRINT RESULT
 if ((EEPROM_pointer < 255)) {
  serial_integer = ((1-negate)*serial_integer + relative_change * ERI(int(EEPROM_pointer*2)));
  EWI(int(EEPROM_pointer*2), int(serial_integer));                     //write the value to EEPPROM, if there is no change, don't write
  EEPROM_check_limits();                                //checks values in EEPROM against the hard coded limits and correct if necessary
  Serial.println(EEPROM_pointer*2);                                    //print the EEPROM start write position
  Serial.print(char(EEPROM_pointer + 59));                             //print the corresponding character
  Serial.println(ERI(int(EEPROM_pointer*2)));                          //print the value in EEPROM after writing
 }


 
}

// EEPROM WRITE INTEGER
void EWI(int adr, int wert) {
  if (((adr==46)|(adr==48)|((adr>58)&(adr<70))) & ((calibration_step==0) | (calibration_step>8 ))){
    if (!(calibration_step==20)){ return;}
  }  // These values may only be changed when a new calibration is started
  byte low, high;
  low=wert&0xFF;                                       // 0xFF == B11111111 selects and copies all bits of the low byte in "wert" to "low"
  high=(wert>>8)&0xFF;                                 // shifts the high byte to the low byte and copies it to "high"
  EEPROM.write(adr, low); // duration 3.3 ms           // write least significant byte
  EEPROM.write(adr+1, high);                           // write most significant byte
  return;
} 

// EEPROM READ INTEGER
int ERI(int adr) {
  byte low, high;
  low=EEPROM.read(adr);                                                 // get "low" from EEPROM
  high=EEPROM.read(adr+1);                                              // get "high"
  return low + ((high << 8)&0xFF00);                                    // assemble and return
} 

// PRINT DATE AND TIME FROM RTC
void RTC_print(){                                                       
     for (byte hs_counter = 7; hs_counter > 0; hs_counter--){           // start reading with highest register
       if (hs_counter == 4) hs_counter --;                              // skip day of week
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          byte received = i2c_read_byte(RTC_ADDRESS,hs_counter-1);         // get byte from RTC
       received = BCD2DEC(received);                                    // convert from BCD to two digit decimal number
       Serial.print(F("\t")); Serial.print(F(":")); Serial.print(received);  //output EEPROM as a string
     }
}

// CHECK EEPROM CONTENTS AGAINS HARD CODED LIMITS AND CORRECT
void EEPROM_check_limits(){                                            
  for (int i_EC = 0; i_EC < 64; i_EC ++){
    if (ERI(2*i_EC) < int(pgm_read_word(&EEPROM_limits[i_EC*3]))) {     // check stored value against hard coded lower limit
      EWI(2*i_EC, int(pgm_read_word(&EEPROM_limits[i_EC*3])));}         // write hard coded lower limit
    if (ERI(2*i_EC) > int(pgm_read_word(&EEPROM_limits[i_EC*3+1]))) {   // check stored value against hard coded upper limit
      EWI(2*i_EC, int(pgm_read_word(&EEPROM_limits[i_EC*3+1])));}       // write hard coded upper limit
  }
}
