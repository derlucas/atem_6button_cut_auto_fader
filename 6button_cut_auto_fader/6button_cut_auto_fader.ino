// firmware for the 6 channel Program/Preview ATEM Television Controller (with Cut+Auto Buttons and Slider)
#include <SPI.h>
#include <Ethernet.h>
#include "Wire.h"
#include "MCP23017.h"

#include <ATEMbase.h>
#include <ATEMstd.h>

MCP23017 buttonMux;

uint16_t _buttonStatus;
uint16_t _buttonStatusLastDown;
uint16_t _lastButtonStatus;
uint16_t _lastButtonReleased;

int _slider_previousValue;
int _slider_previousPosition;
bool _slider_sliderDirectionUp;

byte mac[] = { 0x90, 0xA2, 0xDB, 0x2A, 0x2B, 0xC6 };
IPAddress clientIp(192, 168, 10, 237);
IPAddress atemIp(192, 168, 10, 241);
ATEMstd AtemSwitcher;
bool AtemOnline = false;

bool slider_hasMoved()	{
  int sliderValue = analogRead(A0);

  if (sliderValue >= _slider_previousValue+30 || sliderValue <= _slider_previousValue-30)  {
    // Find direction:
    if (sliderValue >= _slider_previousValue+30 && (_slider_previousValue==-1 || _slider_previousValue<30))  {
      _slider_sliderDirectionUp = true;
    }
    if (sliderValue <= _slider_previousValue-30 && (_slider_previousValue==-1 || _slider_previousValue>1024-30))  {
      _slider_sliderDirectionUp = false;
    }

    _slider_previousValue = sliderValue;

    int transitionPosition = (long)1000*(long)(sliderValue-30)/(long)(1024-30-30);
    
    if (transitionPosition>1000) transitionPosition=1000;
    if (transitionPosition<0) transitionPosition=0;
    if (!_slider_sliderDirectionUp) transitionPosition = 1000-transitionPosition;
    
    if (_slider_previousPosition!=transitionPosition) {
	    bool returnValue = true;
      if ((_slider_previousPosition==0 || _slider_previousPosition==1000) && (transitionPosition==0 || transitionPosition==1000)) {
        returnValue = false;
      }
      _slider_previousPosition=transitionPosition;
      return returnValue;
    }
  }
  return false;
}

uint16_t buttonDownAll() {
  // Reads button status from MCP23017 chip.
  _buttonStatus = buttonMux.digitalWordRead();
  		
  if (_buttonStatus != _lastButtonStatus) {
    _lastButtonReleased = (_lastButtonStatus ^ _buttonStatus) & ~_buttonStatus;
    _lastButtonStatus = _buttonStatus;
  }
	
  uint16_t buttonChange = _buttonStatusLastDown ^ _buttonStatus;
  _buttonStatusLastDown = _buttonStatus;
	
  return buttonChange & _buttonStatus;
}

void handleButtons(uint16_t buttons) {
  if(buttons & 0x100) {
    AtemSwitcher.changeProgramInput(1);
  }
  
  if(buttons & 0x200) {
    AtemSwitcher.changeProgramInput(2);
  }
  
  if(buttons & 0x400) {
    AtemSwitcher.changeProgramInput(3);
  }
  
  if(buttons & 0x800) {
    AtemSwitcher.changeProgramInput(4);
  }
  
  if(buttons & 0x1000) {
    AtemSwitcher.changeProgramInput(5);
  }
  
  if(buttons & 0x2000) {
    AtemSwitcher.changeProgramInput(6);
  }
  
  if(buttons & 0x4000) {
    // cut
    AtemSwitcher.doAuto();
  }
  
  if(buttons & 0x01) {
    AtemSwitcher.changePreviewInput(1);
  }
  
  if(buttons & 0x02) {
    AtemSwitcher.changePreviewInput(2);
  }
  
  if(buttons & 0x04) {
    AtemSwitcher.changePreviewInput(3);
  }
  
  if(buttons & 0x08) {
    AtemSwitcher.changePreviewInput(4);
  }
  
  if(buttons & 0x10) {
    AtemSwitcher.changePreviewInput(5);
  }
  
  if(buttons & 0x20) {
    AtemSwitcher.changePreviewInput(6);
  }
  
  if(buttons & 0x40) {
    AtemSwitcher.doCut();
  }
  
  if(slider_hasMoved()) {
    AtemSwitcher.changeTransitionPosition(_slider_previousPosition*10);
    AtemSwitcher.delay(20);
    if (_slider_previousPosition==1000 || _slider_previousPosition==0) {
      AtemSwitcher.changeTransitionPositionDone();
      AtemSwitcher.delay(5);  
    }
  } 
}

void setup() {
  Wire.begin();
  buttonMux.begin(0);
  buttonMux.inputPolarityMask(0);
  buttonMux.init();
  buttonMux.internalPullupMask(0xffff);	// All has pull-up
  buttonMux.inputOutputMask(0xffff);	// All are inputs.

  _slider_previousValue = -1;
  _slider_previousPosition = -1;
  _slider_sliderDirectionUp = false;
  slider_hasMoved();
  
  DDRD = 0xFC;  // LED port as Output
  
  Serial.begin(57600);  
  Ethernet.begin(mac, clientIp);
  AtemSwitcher.begin(atemIp, 55555);
  AtemSwitcher.connect();

  for(uint8_t i=0;i<6;i++) {
    PORTD = 1<<i+2;
    delay(100);
  }
  PORTD = 0;
  
  buttonDownAll();  // do this once to avoid wrong results
}

void setLEDs() {
  for(uint8_t i=0;i<6;i++) {    
      if(AtemSwitcher.getTallyByIndexTallyFlags(i) & 0x01) {
        digitalWrite(i+2, HIGH);
      } else {
        digitalWrite(i+2, LOW);
      }
   }
}


void loop() {  
  AtemSwitcher.runLoop();    
  
  if (AtemSwitcher.hasInitialized())  {
    if (!AtemOnline)  {
      AtemOnline = true;          
      // we can do here stuff once we connected
      Serial.println("atem online");
    }

    handleButtons(buttonDownAll());
    setLEDs();
  } else {
    // at this point the ATEM is not connected and initialized anymore

    if (AtemOnline) {
      AtemOnline = false;

      // turn off the green connection idicator
      //digitalWrite(STATUS_LED, LOW);
      Serial.println("atem offline");
    }
  }
}
