#include <SPI.h>
#include <RAS.h> // Rugged Audio Shield library

//###### Rugged Audio State Variables ######
RAS RAS;                             // Declare the Rugged Audio Shield object  
int lastVolume;                      //Last volume read from the potentiometer

//Tell us whether we should record sound or if we already are
char FILE_LETTERS[] = {
   'A', 'B', 'C', 'D',  'E',  'F',  'G',  'H',  'I',
   'J', 'K', 'L', 'M',  'N',  'O',  'P',  'Q',  'R', 
   'S', 'T', 'U', 'V',  'W',  'X',  'Y',  'Z'};

//###### Recording State Variables ######
boolean SHOULD_RECORD_AUDIO;         //Just started recording?
boolean IS_RECORDING;                //Are we recording?
int MAX_RECORD_TIME = 10;            //Max Number of seconds to record

unsigned long currentTime;           //Time since the Program has been running
unsigned long startRecordTime;       //Time at which recording has started

//###### File Saving State Variables ######
String WAV_PREPEND_NAME;
int randStartDigit;                  //Start Folder name for saving files
int recordCount;                     //Number of recorded sounds this session

//###### Serial Monitor ######
int serialPort = 9600;               //Serial port for receiving stop/start cmds

//###### Streaming State Variables ######
boolean IS_STREAMING = false;


//###### Initial Set Up ######
void setup(void) {
  //Initialize Variables
  SHOULD_RECORD_AUDIO = false;
  IS_RECORDING = false;
  IS_STREAMING = false;
  recordCount = 0;
  //File name can only be 8 characters, .+wav => 4, so 4 left
  randStartDigit = (random(0,12058675)*random(0,129579))%26;
  //Create a File prepending name for output
  WAV_PREPEND_NAME = String(FILE_LETTERS[randStartDigit]);
  
  // PLAYBACK AUDIO
  RAS.begin();                     // Default SPI select on D8
  Serial.begin(serialPort);  
  // Adjust these to 1X, 2X, 4X, or 8X depending on your source material
  RAS.SetInputGainMic(INPUT_GAIN_2X); delay(10);
  //RAS.SetInputGainLine(INPUT_GAIN_1X); delay(10);
  RAS.OutputEnable(); delay(10);
   
  //RAS.WaitForIdle();
  RAS.InitSD(/*SPI_RATE_4MHz*/);
  
  //Serial.println("Playing Live audio..");
  Serial.println("Press 'o' to stream live audio...");
  Serial.println("Press 'r' to record a sample...");
}

//###### App Loop ######
void loop(void) {
  //Update the current Time
  currentTime = millis();
  //Current key pressed 
 if (Serial.available() > 0) { 
    char pressed = Serial.read();
    checkRecordingStatus(pressed);        //Check status of recording - should we start or stop?
    checkStreamingStatus(pressed);        //Check if we need to start or stop streaming
 }
  //Update the volume if necessary
  updateVolume(); 
}
 
 
 
//###### Check Streaming Status ###### 
void checkStreamingStatus(char pressed) {
   if (pressed == 'o') {
     startStreaming(); 
   }
   else if (pressed == 'p') {
    stopStreaming(); 
   }
}
   
 
 //###### Stop Streaming ######
void stopStreaming() {
  if (IS_STREAMING) {
    RAS.Stop();
    IS_STREAMING = false;
    Serial.println("Stopped Streaming - press 'o' to start streaming");
  }
}
//###### Start Streaming ######
void startStreaming(){
  if (!IS_STREAMING) {
    RAS.AudioEffect(EFFECT_NONE, 32000, SOURCE_STEREO, SOURCE_MIC);delay(10);
    IS_STREAMING = true;
    Serial.println("Streaming - press 'p' to stop streaming");
  }
}
  
//###### Check Recording Status ######
void checkRecordingStatus(char pressed) {
  if (pressed == 'r' && !IS_RECORDING)     startRecording();
  else if (pressed == 's' && IS_RECORDING) stopRecording();
  else if (IS_RECORDING) {
    unsigned long timeRecording = abs(currentTime-startRecordTime)/1000;
    if (timeRecording >= MAX_RECORD_TIME) {
      Serial.println("Reached Max Recording Time!");
      stopRecording();
    }
  }

}
 
//###### StartRecording ######
void startRecording() {

  IS_RECORDING = true;
  SHOULD_RECORD_AUDIO = true;
  startRecordTime = millis();
  //Start recording to file
  String recordName = (recordCount <10) ? "0"+String(recordCount) : String(recordCount);
  String fileName;
  if( recordCount < 10) {
    fileName = WAV_PREPEND_NAME + "00" + String(recordCount) + ".WAV";
  } else if (10 <= recordCount || recordCount < 100) {
    fileName = WAV_PREPEND_NAME + "0" + String(recordCount) + ".WAV";
  } else if (100 <= recordCount || recordCount < 1000) {
    fileName = WAV_PREPEND_NAME + String(recordCount) + ".WAV";
  } else {
    stopRecording();
    Serial.println("Cannot record - too many files");
    return;
  }  
  char charBuf[15];
  fileName.toCharArray(charBuf, 15);
  RAS.RecordWAV(32000, SOURCE_MONO, SOURCE_MIC, charBuf);
  //delay(MAX_RECORD_TIME*1000);
  Serial.println("Recording as " + fileName + "- Press 's' to stop.");
  Serial.println(fileName);
  Serial.println(charBuf);
}

//###### Stop recording ######
void stopRecording() {
  Serial.println("Stopped Recording - Press 'r' to start.");
  IS_RECORDING = false;
  SHOULD_RECORD_AUDIO = false;
  recordCount++;
  //Stop the recording
  RAS.Stop(); RAS.WaitForIdle();
  //Keep Streaming if we already are
  if (IS_STREAMING) {
    RAS.AudioEffect(EFFECT_NONE, 32000, SOURCE_STEREO, SOURCE_MIC); delay(10);
  }
  
}

//###### Update Volume ######
//Updates the volume from the potentiometer reading
void updateVolume() {
  int reading;
    // Update the volume every 300ms (not to overwhelm the SPI bus), and
    // don't bother sending a volume update unless the potentiometer reading
    // changed by more than 16 counts.
    delay(300);
    reading = analogRead(3); // Read shield potentiometer
    if (abs(reading - lastVolume) > 16) {
      lastVolume = reading;
      // Potentiometer reads 0V to 3.3V, which is 0 to about 700
      // when using the default 5V analog reference. So we map the range
      // 0<-->700 to the OutputVolumeSet() range of 0 to 31.
      RAS.OutputVolumeSet(map(reading, 0, 700, 0, 31));
    }   
}
