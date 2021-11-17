class ornament{
  public:
    int LED;
    long interval = 0;
    long previousMillis = 0;
    bool ledStatus = false;
    String ornName = "";

    void turnOn () {
		digitalWrite(LED, HIGH);
    	ledStatus = true;
    }  
    void turnOff () {
    	digitalWrite(LED, LOW);
    	ledStatus = false;
    }
    void Setup() {
        pinMode(LED,OUTPUT);
		turnOff();
		ledStatus = false;
    }
    void toggle() {     
        ledStatus ? turnOff() : turnOn();
    }
    void setInterval(bool debugMode) {
		if (debugMode) {
			interval = random(1000, 5000);
		} else {
			interval = random(300000, 500000);
		}
    }

    bool checkStatus(bool debugMode) { 
      // save the last time you blinked the LED 
      unsigned long currentMillis = millis();

      if(currentMillis - previousMillis  > interval) {
        previousMillis = currentMillis; 
        toggle();
        setInterval(debugMode);
		return true;
    } else {
		return false;
	}
  }
};


