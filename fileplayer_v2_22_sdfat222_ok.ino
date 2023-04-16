/**
 * \file FilePlayer.ino
 *  
 * \brief Example sketch of using the vs1053 Arduino driver, with flexible list of files and formats
 * \remarks comments are implemented with Doxygen Markdown format
 *
 * \author Bill Porter
 * \author Michael P. Flaga with sdfat v1.1.4
 * \author Dusan Kover modify for support MP3 ID tag 1.0 and 2.0 and 3.0 and new Libary sdfat2.2.2
           Encoding of file names, folders and ID3 text is in UTF8 
 *         Thank you very much for the support and advice Bill Greiman (SdFat) & Michael P. Flaga
 * \ beta verzia v2.22, working on atmega2560

 /** Change in SdFatConfig, libary SDFAT/scr SdFatConfig.h
/*
// To try UTF-8 encoded filenames.
#define USE_UTF8_LONG_NAMES 0 <-------------------------- change for 0 to 1 

#ifndef SDFAT_FILE_TYPE
#if defined(__AVR__) && FLASHEND < 0X8000
// 32K AVR boards.
#define SDFAT_FILE_TYPE 1
#else  // defined(__AVR__) && FLASHEND < 0X8000
// All other boards.
#define SDFAT_FILE_TYPE 3 <------------------------------- change for 3 to 1
#endif  // defined(__AVR__) && FLASHEND < 0X8000
#endif  // SDFAT_FILE_TYPE
*/
/*
 * This sketch listens for commands from a serial terminal (such as the Serial
 * Monitor in the Arduino IDE). Listening for either a single character menu
 * commands or an numeric strings of an index. Pointing to a music file, found
 * in the root of the SdCard, to be played. A list of index's and corresponding
 * files in the root can be listed out using the 'l' (little L) command.
 *
 * This sketch allows the various file formats to be played: mp3, aac, wma, wav,
 * fla & mid.
 *
 * This sketch behaves nearly identical to vs1053_Library_Demo.ino, but has
 * extra complicated loop() as to recieve string of characters to create the
 * file index. As the Serial Monitor is typically default with no CR or LF, this
 * sketch uses intercharacter time out as to determine when a full string has
 * has been entered to be processed.
 */

#include <SPI.h>

//Add the SdFat Libraries
#include <SdFat.h>          //verzia 2.2.2 with modified
#include <FreeStack.h>

//and the MP3 Shield Library
#include "vs1053_SdFat.h"

// Below is not needed if interrupt driven. Safe to remove if not using.
#if defined(USE_MP3_REFILL_MEANS) && USE_MP3_REFILL_MEANS == USE_MP3_Timer1
  #include <TimerOne.h>
#elif defined(USE_MP3_REFILL_MEANS) && USE_MP3_REFILL_MEANS == USE_MP3_SimpleTimer
  #include <SimpleTimer.h>
#endif
//***************************************************************************
// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(50)
/*
// SDCARD_SS_PIN is defined for the built-in SD on some boards.
#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = 9;
#else   // SDCARD_SS_PIN
// Assume built-in SD is used.
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif  // SDCARD_SS_PIN
*/
const uint8_t SD_CS_PIN = SD_SEL;
//next pin defined in vs1053_SdFat_config.h
/*
// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif  // HAS_SDIO_CLASS
*/
#define SD_CONFIG SdSpiConfig(SD_SEL, SHARED_SPI, SPI_CLOCK)
//----------------------------------------------------------------------------
// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 0
#if SD_FAT_TYPE == 0
SdFat sd;
File dir;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 dir;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile dir;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile dir;
FsFile file;
#else  // SD_FAT_TYPE
#error invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE
//------------------------------------------------------------------------------
// Store error strings in flash to save RAM.
#define error(s) sd.errorHalt(&Serial, F(s))
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


/**
 * \brief Object instancing the vs1053 library.
 *
 * principal object for handling all the attributes, members and functions for the library.
 */
vs1053 MP3player;
int16_t last_ms_char; // milliseconds of last recieved character from Serial port.
//int16_t wait_ms_char; // milliseconds of Play all files.
int8_t buffer_pos;    // next position to recieve character from Serial port.
int8_t fileindir;     // file in dir, "l" command, for playAll
int8_t PlayAll;       // file in dir, "l" command, for PlayAll

//------------------------------------------------------------------------------
/**
 * \brief Setup the Arduino Chip's feature for our use.
 *
 * After Arduino's kernel has booted initialize basic features for this
 * application, such as Serial port and MP3player objects with .begin.
 * Along with displaying the Help Menu.
 *
 * \note returned Error codes are typically passed up from MP3player.
 * Whicn in turns creates and initializes the SdCard objects.
 *
 * \see
 * \ref Error_Codes
 */
  char buffer[6];           // 0-35K+null
  char dirbuffer[60];       // for name of directory temporary
 // char dirbuffer2[60]="/";  // for name of directory
  char dirbuffer2[60]="/Ceske hity/"; // for name of directory
  
void setup() {

  uint8_t result; //result code from some function as to be tested at later time.

  Serial.begin(115200);

  Serial.print(F("F_CPU = "));
  Serial.println(F_CPU);
  Serial.print(F("Free RAM = ")); // available in Version 1.0 F() bases the string to into Flash, to use less SRAM.
  Serial.print(FreeStack(), DEC);  // FreeRam() is provided by SdFatUtil.h
  Serial.println(F(" Should be a base line of 1017, on ATmega328 when using INTx"));

  // Initialize the SD.
  if (!sd.begin(SD_CONFIG)) {
    sd.initErrorHalt(&Serial);
  }
     // Open root directory
  if (!dir.open("/")) {
    error("dir.open failed");
  }
 
  //Initialize the MP3 Player Shield
  result = MP3player.begin();
  //check result, see readme for error codes.
  if(result != 0) {
    Serial.print(F("Error code: "));
    Serial.print(result);
    Serial.println(F(" when trying to start MP3 player"));
    if( result == 6 ) {
      Serial.println(F("Warning: patch file not found, skipping.")); // can be removed for space, if needed.
      Serial.println(F("Use the \"d\" command to verify SdCard can be read")); // can be removed for space, if needed.
    }
  }
    MP3player.setVolume(0,0); // commit new volume
 //     MP3player.setVolume(70,70); // commit new volume sluchatka
/*
#if (0)
  // Typically not used by most shields, hence commented out.
  Serial.println(F("Applying ADMixer patch."));
  if(MP3player.ADMixerLoad("admxster.053") == 0) {
    Serial.println(F("Setting ADMixer Volume."));
    MP3player.ADMixerVol(-3);
  }
#endif
*/
  help();
  last_ms_char = millis(); // stroke the inter character timeout.
  //wait_ms_char = millis(); // stroke the inter character timeout.
  buffer_pos = 0; // start the command string at zero length.
  //parse_menu('q'); // display the list of directory
  parse_menu('l'); // display the list of files to play
}

//------------------------------------------------------------------------------
/**
 * \brief Main Loop the Arduino Chip
 *
 * This is called at the end of Arduino kernel's main loop before recycling.
 * And is where the user's serial input of bytes are read and analyzed by
 * parsed_menu.
 *
 * Additionally, if the means of refilling is not interrupt based then the
 * MP3player object is serviced with the availaible function.
 *
 * \note Actual examples of the libraries public functions are implemented in
 * the parse_menu() function.
 */
void loop() {

// Below is only needed if not interrupt driven. Safe to remove if not using.
#if defined(USE_MP3_REFILL_MEANS) \
    && ( (USE_MP3_REFILL_MEANS == USE_MP3_SimpleTimer) \
    ||   (USE_MP3_REFILL_MEANS == USE_MP3_Polled)      )
  MP3player.available();
#endif

  char inByte;
  if (Serial.available() > 0) {
    inByte = Serial.read();
    if ((0x20 <= inByte) && (inByte <= 0x126)) { // strip off non-ASCII, such as CR or LF
      if (isDigit(inByte)) { // macro for ((inByte >= '0') && (inByte <= '9'))
        // else if it is a number, add it to the string
        buffer[buffer_pos++] = inByte;
      } else {
        // input char is a letter command
        buffer_pos = 0;
        parse_menu(inByte);
      }
      buffer[buffer_pos] = 0; // update end of line
      last_ms_char = millis(); // stroke the inter character timeout.
    }
  } else if ((millis() - last_ms_char) > 50 && ( buffer_pos > 0 )) {
    // ICT expired and have something
    if (buffer_pos == 1) {
      // look for single byte (non-number) menu commands
      parse_menu(buffer[buffer_pos - 1]);

    } else if (buffer_pos > 5) {
      // dump if entered command is greater then uint16_t
      Serial.println(F("Ignored, Number is Too Big!"));
  //------------------------------------------------------------------------------
    //open directory
    // otherwise its a number, scan through files looking for matching index.  
  //-------------------------------------------------------------------------
    } else {
      int16_t fn_index = atoi(buffer);
      if (fn_index>9999)
      {
      if (fn_index==10000){memset(dirbuffer2, 0, sizeof(dirbuffer2));
                          dirbuffer2[0]='/';Serial.println(dirbuffer2);
                          sd.chdir(dirbuffer2);dir.open(dirbuffer2);
                          }
            else
            {
            File dir;
            char dirname[60];
            sd.chdir(dirbuffer2);
            dir.open(dirbuffer2);
            dir.rewind();

            uint16_t count = 10001;
            while (file.openNext(&dir, O_RDONLY))
                  {
                  if (file.isDir()){ 
                    file.getName(dirname, sizeof(dirname));
                    if (  dirname>0 ) {
                          if (count == fn_index) {
                          //Serial.print(F("Directory is: "));
                          Serial.println(dirname);
                          break;
                          }
                      count++;
                    }
                  }
                  file.close();
                }
              file.close();
      // end vyberu direktory

              Serial.println(F("Change directory to"));
              String(dirbuffer)=("/" + String(dirname) + "/");
              Serial.println(dirbuffer);
              dirbuffer.toCharArray(dirbuffer2,60);
              sd.chdir(dirbuffer2);
              dir.open(dirbuffer2);
              }
      } //end (fn_index>9999)
      else
          {
          // otherwise its a number, scan through files looking for matching index.
          File dir;
          char filename[60];
          sd.chdir(dirbuffer2);
          dir.open(dirbuffer2);
          dir.rewind();
          uint16_t count = 1;

          while (file.openNext(&dir, O_RDONLY))
            {
              file.getName(filename, sizeof(filename));
              if ( isFnMusic(filename) ) {
                if (count == fn_index) {
 //             Serial.print(F("Index "));
 //             SerialPrintPaddedNumber(count, 5 );
 //             Serial.print(F(": "));
 //             Serial.println(filename);
                Serial.print(F("Playing filename: "));
                Serial.println(filename);
                int8_t result = MP3player.playMP3(filename);
                //check result, see readme for error codes.
                if(result != 0) {
                  Serial.print(F("Error code: "));
                  Serial.print(result);
                  Serial.println(F(" when trying to play track"));
                  }

                char ID[4]; // buffer to id3
                //test ID3v1 or ID3v2 or ID3v2
                MP3player.trackID3((char*)&ID);
                break;
                }
              count++;
              } 
            file.close();
            }
          }
    }
    //reset buffer to start over
    buffer_pos = 0;
    buffer[buffer_pos] = 0; // delimit
  }
  delay(10);    //bolo 100
  //----------------------------------------------------------------
// play all files to directory
      if(!MP3player.isPlaying()&&(PlayAll)) {
        delay(100);    //bolo 100
        Serial.print(F("Files in directory:"));
        Serial.println(fileindir);

        if (PlayAll<=fileindir)
          {        
          Serial.print(F("Play all files, play file number:"));
          Serial.println(PlayAll);
          sprintf (buffer, "%02i", PlayAll);
          Serial.println(buffer);
          buffer_pos = 2;
          PlayAll++;
          }else {PlayAll=0;buffer_pos = 0;}  //end play all file to dir
      }
//--------------------------------------------------------------------

}   //end LOOP

//uint32_t  millis_prv;

//------------------------------------------------------------------------------
/**
 * \brief Decode the Menu.
 *
 * Parses through the characters of the users input, executing corresponding
 * MP3player library functions and features then displaying a brief menu and
 * prompting for next input command.
 */
void parse_menu(byte key_command) {

  uint8_t result; // result code from some function as to be tested at later time.

  // Note these buffer may be desired to exist globably.
  // but do take much space if only needed temporarily, hence they are here.
  char title[30]; // buffer to contain the extract the Title from the current filehandles
  char artist[30]; // buffer to contain the extract the artist name from the current filehandles
  char album[30]; // buffer to contain the extract the album name from the current filehandles

  Serial.print(F("Received command: "));
  Serial.write(key_command);
  Serial.println(F(" "));

  //if s, stop the current track
  if(key_command == 's') {
    Serial.println(F("Stopping"));
    PlayAll=0;
    MP3player.stopTrack();

  /* Display the file on the SdCard */
  } else if(key_command == 'd') {
    if(!MP3player.isPlaying()) {
      // prevent root.ls when playing, something locks the dump. but keeps playing.
      // yes, I have tried another unique instance with same results.
      // something about SdFat and its 500byte cache.
      Serial.println(F("Files found (name date time size):"));
//    sd.ls(LS_R | LS_DATE | LS_SIZE);
      sd.ls(LS_R);  //len text suboru
    } else {Serial.println(F("Busy Playing Files, try again later."));}
//----------------------------------------------------------------------------------
  /* Display the file on the SdCard */
  } else if(key_command == 'w') {
    if(!MP3player.isPlaying()) {
      // prevent root.ls when playing, something locks the dump. but keeps playing.
      // yes, I have tried another unique instance with same results.
      // something about SdFat and its 500byte cache.
      Serial.println(F("Directory found:"));
//    sd.ls(LS_R | LS_DATE | LS_SIZE);
      sd.ls(dirbuffer2); 
    } else {Serial.println(F("Busy Playing Files, try again later."));}
//***********************************************************************************
  /* List out directory on the SdCard */
  
  } else if(key_command == 'q') {

    Serial.println(F("Stopping"));
    MP3player.stopTrack();
    
    if(!MP3player.isPlaying()) {
      Serial.println(F("Directory and files found :"));

      File dir;
      char filename[60];
      sd.chdir(dirbuffer2);
      dir.open(dirbuffer2);
      dir.rewind();
      uint16_t count = 10001;

      while (file.openNext(&dir, O_RDONLY))
      { 
        if (file.isDir()){ 
          file.getName(filename, sizeof(filename));
          if (filename>0) {
            SerialPrintPaddedNumber(count, 5 );
            Serial.print(F(": "));
            Serial.println(filename);
            count++;
          }   
        }  
      file.close();
      }
      Serial.println(F("Enter Index of directory"));
    } else {Serial.println(F("Busy Playing Files, try again later."));}
//*************************************************************************

  /* Get and Display the Audio Information */
  } else if(key_command == 'i') {
    MP3player.getAudioInfo();

  } else if(key_command == 'p') {
    if( MP3player.getState() == playback) {
      MP3player.pauseMusic();
      Serial.println(F("Pausing"));
    } else if( MP3player.getState() == paused_playback) {
      MP3player.resumeMusic();
      Serial.println(F("Resuming"));
    } else {
      Serial.println(F("Not Playing!"));
    }

  } else if(key_command == 't') {
    int8_t teststate = MP3player.enableTestSineWave(126);
    if(teststate == -1) {
      Serial.println(F("Un-Available while playing music or chip in reset."));
    } else if(teststate == 1) {
      Serial.println(F("Enabling Test Sine Wave"));
    } else if(teststate == 2) {
      MP3player.disableTestSineWave();
      Serial.println(F("Disabling Test Sine Wave"));
    }

  } else if(key_command == 'S') {
    Serial.println(F("Current State of VS10xx is."));
    Serial.print(F("isPlaying() = "));
    Serial.println(MP3player.isPlaying());

    Serial.print(F("getState() = "));
    switch (MP3player.getState()) {
    case uninitialized:
      Serial.print(F("uninitialized"));
      break;
    case initialized:
      Serial.print(F("initialized"));
      break;
    case deactivated:
      Serial.print(F("deactivated"));
      break;
    case loading:
      Serial.print(F("loading"));
      break;
    case ready:
      Serial.print(F("ready"));
      break;
    case playback:
      Serial.print(F("playback"));
      break;
    case paused_playback:
      Serial.print(F("paused_playback"));
      break;
    case testing_memory:
      Serial.print(F("testing_memory"));
      break;
    case testing_sinewave:
      Serial.print(F("testing_sinewave"));
      break;
    }
    Serial.println();

   } else if(key_command == 'b') {
    Serial.println(F("Playing Static MIDI file."));
    MP3player.SendSingleMIDInote();
    Serial.println(F("Ended Static MIDI file."));

#if !defined(__AVR_ATmega32U4__)
  } else if(key_command == 'm') {
      uint16_t teststate = MP3player.memoryTest();
    if(teststate == -1) {
      Serial.println(F("Un-Available while playing music or chip in reset."));
    } else if(teststate == 2) {
      teststate = MP3player.disableTestSineWave();
      Serial.println(F("Un-Available while Sine Wave Test"));
    } else {
      Serial.print(F("Memory Test Results = "));
      Serial.println(teststate, HEX);
      Serial.println(F("Result should be 0x83FF."));
      Serial.println(F("Reset is needed to recover to normal operation"));
    }

  } else if(key_command == 'e') {
    uint8_t earspeaker = MP3player.getEarSpeaker();
    if(earspeaker >= 3){
      earspeaker = 0;
    } else {
      earspeaker++;
    }
    MP3player.setEarSpeaker(earspeaker); // commit new earspeaker
    Serial.print(F("earspeaker to "));
    Serial.println(earspeaker, DEC);

  } else if(key_command == 'r') {
    MP3player.resumeMusic(2000);

  } else if(key_command == 'R') {
    MP3player.stopTrack();
    MP3player.vs_init();
    Serial.println(F("Reseting VS10xx chip"));

  } else if(key_command == 'g') {
    int32_t offset_ms = 20000; // Note this is just an example, try your own number.
    Serial.print(F("jumping to "));
    Serial.print(offset_ms, DEC);
    Serial.println(F("[milliseconds]"));
    result = MP3player.skipTo(offset_ms);
    if(result != 0) {
      Serial.print(F("Error code: "));
      Serial.print(result);
      Serial.println(F(" when trying to skip track"));
    }

  } else if(key_command == 'k') {
    int32_t offset_ms = -1000; // Note this is just an example, try your own number.
    Serial.print(F("moving = "));
    Serial.print(offset_ms, DEC);
    Serial.println(F("[milliseconds]"));
    result = MP3player.skip(offset_ms);
    if(result != 0) {
      Serial.print(F("Error code: "));
      Serial.print(result);
      Serial.println(F(" when trying to skip track"));
    }

  } else if(key_command == 'O') {
    MP3player.end();
    Serial.println(F("VS10xx placed into low power reset mode."));

  } else if(key_command == 'o') {
    MP3player.begin();
    Serial.println(F("VS10xx restored from low power reset mode."));

  } else if(key_command == 'D') {
    uint16_t diff_state = MP3player.getDifferentialOutput();
    Serial.print(F("Differential Mode "));
    if(diff_state == 0) {
      MP3player.setDifferentialOutput(1);
      Serial.println(F("Enabled."));
    } else {
      MP3player.setDifferentialOutput(0);
      Serial.println(F("Disabled."));
    }

  } else if(key_command == 'V') {
    MP3player.setVUmeter(1);
    Serial.println(F("Use \"No line ending\""));
    Serial.print(F("VU meter = "));
    Serial.println(MP3player.getVUmeter());
    Serial.println(F("Hit Any key to stop."));

    while(!Serial.available()) {
      union twobyte vu;
      vu.word = MP3player.getVUlevel();
      Serial.print(F("VU: L = "));
      Serial.print(vu.byte[1]);
      Serial.print(F(" / R = "));
      Serial.print(vu.byte[0]);
      Serial.println(" dB");
      delay(1000);
    }
    Serial.read();

    MP3player.setVUmeter(0);
    Serial.print(F("VU meter = "));
    Serial.println(MP3player.getVUmeter());

  } else if(key_command == 'T') {
    uint16_t TrebleFrequency = MP3player.getTrebleFrequency();
    Serial.print(F("Former TrebleFrequency = "));
    Serial.println(TrebleFrequency, DEC);
    if (TrebleFrequency >= 15000) { // Range is from 0 - 1500Hz
      TrebleFrequency = 0;
    } else {
      TrebleFrequency += 1000;
    }
    MP3player.setTrebleFrequency(TrebleFrequency);
    Serial.print(F("New TrebleFrequency = "));
    Serial.println(MP3player.getTrebleFrequency(), DEC);

  } else if(key_command == 'E') {
    int8_t TrebleAmplitude = MP3player.getTrebleAmplitude();
    Serial.print(F("Former TrebleAmplitude = "));
    Serial.println(TrebleAmplitude, DEC);
    if (TrebleAmplitude >= 7) { // Range is from -8 - 7dB
      TrebleAmplitude = -8;
    } else {
      TrebleAmplitude++;
    }
    MP3player.setTrebleAmplitude(TrebleAmplitude);
    Serial.print(F("New TrebleAmplitude = "));
    Serial.println(MP3player.getTrebleAmplitude(), DEC);

  } else if(key_command == 'B') {
    uint16_t BassFrequency = MP3player.getBassFrequency();
    Serial.print(F("Former BassFrequency = "));
    Serial.println(BassFrequency, DEC);
    if (BassFrequency >= 150) { // Range is from 20hz - 150hz
      BassFrequency = 0;
    } else {
      BassFrequency += 10;
    }
    MP3player.setBassFrequency(BassFrequency);
    Serial.print(F("New BassFrequency = "));
    Serial.println(MP3player.getBassFrequency(), DEC);

  } else if(key_command == 'C') {
    uint16_t BassAmplitude = MP3player.getBassAmplitude();
    Serial.print(F("Former BassAmplitude = "));
    Serial.println(BassAmplitude, DEC);
    if (BassAmplitude >= 15) { // Range is from 0 - 15dB
      BassAmplitude = 0;
    } else {
      BassAmplitude++;
    }
    MP3player.setBassAmplitude(BassAmplitude);
    Serial.print(F("New BassAmplitude = "));
    Serial.println(MP3player.getBassAmplitude(), DEC);

  } else if(key_command == 'M') {
    uint16_t monostate = MP3player.getMonoMode();
    Serial.print(F("Mono Mode "));
    if(monostate == 0) {
      MP3player.setMonoMode(1);
      Serial.println(F("Enabled."));
    } else {
      MP3player.setMonoMode(0);
      Serial.println(F("Disabled."));
    }
#endif

  /* List out music files on the SdCa
  rd */
  } else if(key_command == 'l') {
    
    Serial.println(F("Stopping"));
    MP3player.stopTrack();

    if(!MP3player.isPlaying()) {
      Serial.println(F("Music Files found :"));

      File dir;
      char filename[60];
      sd.chdir(dirbuffer2);
      dir.open(dirbuffer2);
      dir.rewind();
 
      uint16_t count = 1;
      while (file.openNext(&dir, O_RDONLY))
        {
        file.getName(filename, sizeof(filename));
        if ( isFnMusic(filename) ) {
          SerialPrintPaddedNumber(count, 5 );
          Serial.print(F(": "));
          Serial.println(filename);
          fileindir=count;
          count++;      
          }
        file.close();
        }

      if (dir.getError()) {Serial.println("openNext failed"); } else {Serial.println("Done!");}
      Serial.println(F("Enter Index of File to play"));
      } else {Serial.println(F("Busy Playing Files, try again later."));}

  } else if(key_command == 'a') {
  Serial.println(F("Play ALL files to dir"));
  PlayAll=1;
  //last_ms_char = millis(); // stroke the inter character timeout.
  //delay(60);
  } else if(key_command == '>') {
  Serial.println(F("Play next files to dir"));
  MP3player.stopTrack();
 // wait_ms_char = millis(); // stroke the inter character timeout.
  //if (PlayAll==fileindir) PlayAll=0;

  } else if(key_command == '<') {
  Serial.println(F("Play preview files to dir"));
  MP3player.stopTrack();
  if (PlayAll==2) PlayAll=1;
  if (PlayAll>2) PlayAll=PlayAll-2;
 // wait_ms_char = millis(); // stroke the inter character timeout.

  } else if(key_command == 'h') {
    help();
  }


 
/*
  // print prompt after key stroke has been processed.
  Serial.print(F("Time since last command: "));  
  Serial.println((float) (millis() -  millis_prv)/1000, 2);  
  millis_prv = millis();
  Serial.print(F("Enter s,1-9,f,F,d,i,p,t,S,b"));
#if !defined(__AVR_ATmega32U4__)
  Serial.print(F(",m,e,r,R,g,k,O,o,D,V,B,C,T,E,M:"));
#endif
  Serial.println(F(",l,h :"));
 */ 
}

//------------------------------------------------------------------------------
/**
 * \brief Print Help Menu.
 *
 * Prints a full menu of the commands available along with descriptions.
 */
void help() {
  Serial.println(F("COMMANDS:"));
  Serial.println(F(" [a] Play ALL files to dir"));
  Serial.println(F(" [>] Play next file to dir"));
  Serial.println(F(" [<] Play next file to dir"));
  Serial.println(F(" [F] same as [f] but with initial skip of 2 second"));
  Serial.println(F(" [s] to stop playing"));
  Serial.println(F(" [d] display directory of SdCard LS"));
  Serial.println(F(" [w] Display the file on the SdCard LS"));
//  Serial.println(F(" [+ or -] to change volume"));
//  Serial.println(F(" [> or <] to increment or decrement play speed by 1 factor"));
  Serial.println(F(" [i] retrieve current audio information (partial list)"));
  Serial.println(F(" [p] to pause."));
  Serial.println(F(" [t] to toggle sine wave test"));
  Serial.println(F(" [S] Show State of Device."));
  Serial.println(F(" [b] Play a MIDI File Beep"));
#if !defined(__AVR_ATmega32U4__)
  Serial.println(F(" [e] increment Spatial EarSpeaker, default is 0, wraps after 4"));
  Serial.println(F(" [m] perform memory test. reset is needed after to recover."));
  Serial.println(F(" [M] Toggle between Mono and Stereo Output."));
  Serial.println(F(" [g] Skip to a predetermined offset of ms in current track."));
  Serial.println(F(" [k] Skip a predetermined number of ms in current track."));
  Serial.println(F(" [r] resumes play from 2s from begin of file"));
  Serial.println(F(" [R] Resets and initializes VS10xx chip."));
  Serial.println(F(" [O] turns OFF the VS10xx into low power reset."));
  Serial.println(F(" [o] turns ON the VS10xx out of low power reset."));
  Serial.println(F(" [D] to toggle SM_DIFF between inphase and differential output"));
  Serial.println(F(" [V] Enable VU meter Test."));
  Serial.println(F(" [B] Increament bass frequency by 10Hz"));
  Serial.println(F(" [C] Increament bass amplitude by 1dB"));
  Serial.println(F(" [T] Increament treble frequency by 1000Hz"));
  Serial.println(F(" [E] Increament treble amplitude by 1dB"));
#endif
  Serial.println(F(" [q] List folders"));
  Serial.println(F(" [l] Display list of music files"));
  Serial.println(F(" [0###] Enter index of file to play, zero pad! e.g. 01-9999"));
  Serial.println(F(" [1####] Enter index of open directory, zero pad! e.g. 10000-19999"));
  Serial.println(F(" [10000] Enter to root directory / "));
  Serial.println(F(" [h] this help"));
}

void SerialPrintPaddedNumber(int16_t value, int8_t digits ) {
  int currentMax = 10;
  for (byte i=1; i<digits; i++){
    if (value < currentMax) {
      Serial.print("0");
    }
    currentMax *= 10;
  }
  Serial.print(value);
}
