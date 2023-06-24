#include <TimeLib.h>
#include <dcf77.h>

/**
 * Initialize parameters
 */
int DCF77::dcfPin;
byte DCF77::pulseStart;

volatile unsigned long long DCF77::filledBuffer = 0;
volatile bool DCF77::FilledBufferAvailable= false;
volatile time_t DCF77::filledTimestamp= 0;

int DCF77::bufferPosition = 0;
unsigned long long DCF77::runningBuffer = 0;
unsigned long long DCF77::processingBuffer = 0;

int DCF77::leadingEdge=0;
int DCF77::trailingEdge=0;
int DCF77::PreviousLeadingEdge=0;
bool DCF77::Up= false;

time_t DCF77::latestupdatedTime= 0;
time_t DCF77::previousUpdatedTime= 0;
time_t DCF77::currentTime = 0;
time_t DCF77::processingTimestamp= 0;
time_t DCF77::previousProcessingTimestamp=0;
unsigned char DCF77::CEST=0;
DCF77::ParityFlags DCF77::flags = {0,0,0,0};

/**
 * Start receiving DCF77 information
 */
void DCF77::Start(const int dCF77Pin) 
{
  	leadingEdge           = 0;
	trailingEdge          = 0;
	PreviousLeadingEdge   = 0;
	Up                    = false;
	runningBuffer		  = 0;
	FilledBufferAvailable = false;
	bufferPosition        = 0;
	flags.parityDate      = 0;
	flags.parityFlag      = 0;
	flags.parityHour      = 0;
	flags.parityMin       = 0;
	CEST				  = 0;
	dcfPin				  = dCF77Pin;
	pinMode(dcfPin, INPUT);
	pulseStart = HIGH;
  	attachInterrupt(dcfPin, isr, CHANGE);
	if(xTaskCreate(DCF77::checkTime,"CheckTime",8192,NULL,1,NULL) != pdPASS)
		Serial.println("Task creation failure");
}

/**
 * Stop receiving DCF77 information
 */
void DCF77::Stop(void) 
{
	detachInterrupt(dcfPin);	
}

/**
 * Initialize buffer for next time update
 */
inline void DCF77::bufferinit(void) 
{
	runningBuffer    = 0;
	bufferPosition   = 0;
}

/**
 * Interrupt handler that processes up-down flanks into pulses and stores these in the buffer
 */
void IRAM_ATTR DCF77::isr() {
	int flankTime = millis();
	byte sensorValue = digitalRead(dcfPin);

	// If flank is detected quickly after previous flank up
	// this will be an incorrect pulse that we shall reject
	if ((flankTime-PreviousLeadingEdge)<DCFRejectionTime) {
		Serial.println("rCT ");
		return;
	}
	
	// If the detected pulse is too short it will be an
	// incorrect pulse that we shall reject as well
	if ((flankTime-leadingEdge)<DCFRejectPulseWidth) {
	    Serial.println("rPW ");
		return;
	}
	
	if(sensorValue==pulseStart) {
		if (!Up) {
			// Flank up
			leadingEdge=flankTime;
			Up = true;		                
		} 
	} else {
		if (Up) {
			// Flank down
			trailingEdge=flankTime;
			int difference=trailingEdge - leadingEdge;            
      		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // blinki 		
			if ((leadingEdge-PreviousLeadingEdge) > DCFSyncTime) {
				finalizeBuffer();
			}         
			PreviousLeadingEdge = leadingEdge;       
			// Distinguish between long and short pulses
			if (difference < DCFSplitTime) { appendSignal(0); } else { appendSignal(1); }
			Up = false;	 
		}
	}  
}

/**
 * Add new bit to buffer
 */
inline void DCF77::appendSignal(unsigned char signal) {
	Serial.print(signal, DEC); Serial.print(" ");
	runningBuffer = runningBuffer | ((unsigned long long) signal << bufferPosition);  
	bufferPosition++;
	if (bufferPosition > 59) {
		// Buffer is full before at end of time-sequence 
		// this may be due to noise giving additional peaks
		Serial.println("EoB");
		finalizeBuffer();
	}
}

void DCF77::finalizeBuffer(void) {
  if (bufferPosition == 59) {
		// Buffer is full
		Serial.println("BF");
		// Prepare filled buffer and time stamp for main loop
		filledBuffer = runningBuffer;
		filledTimestamp = now();
		// Reset running buffer
		bufferinit();
		FilledBufferAvailable = true;    
    } else {
		// Buffer is not yet full at end of time-sequence
		Serial.println("EoM");
		// Reset running buffer
		bufferinit();      
    }
}

/**
 * Returns whether there is a new time update available
 * This functions should be called prior to getTime() function.
 */
bool DCF77::receivedTimeUpdate(void) {
	// If buffer is not filled, there is no new time
	if(!FilledBufferAvailable) {
		return false;
	}
	// if buffer is filled, we will process it and see if this results in valid parity
	if (!processBuffer()) {
		Serial.println("Invalid parity");
		return false;
	}
	
	// Since the received signal is error-prone, and the parity check is not very strong, 
	// we will do some sanity checks on the time
	time_t processedTime = latestupdatedTime + (now() - processingTimestamp);
	if (processedTime<MIN_TIME || processedTime>MAX_TIME) {
		Serial.println("Time outside of bounds");
		return false;
	}

	// If received time is close to internal clock (2 min) we are satisfied
	time_t difference = abs(processedTime - now());
	if(difference < 2*SECS_PER_MIN) {
		Serial.println("close to internal clock");
		storePreviousTime();
		return true;
	}

	// Time can be further from internal clock for several reasons
	// We will check if lag from internal clock is consistent
	time_t shiftPrevious = (previousUpdatedTime - previousProcessingTimestamp);
	time_t shiftCurrent = (latestupdatedTime - processingTimestamp);	
	time_t shiftDifference = abs(shiftCurrent-shiftPrevious);
	storePreviousTime();
	if(shiftDifference < 2*SECS_PER_MIN) {
		Serial.println("time lag consistent");		
		return true;
	} else {
		Serial.println("time lag inconsistent");
	}
	
	// If lag is inconsistent, this may be because of no previous stored date 
	// This would be resolved in a second run.
	return false;
}

/**
 * Evaluates the information stored in the buffer. This is where the DCF77
 * signal is decoded 
 */
bool DCF77::processBuffer(void) {	
	
	/////  Start interaction with interrupt driven loop  /////
	
	// Copy filled buffer and timestamp from interrupt driven loop
	processingBuffer = filledBuffer;
	processingTimestamp = filledTimestamp;
	// Indicate that there is no filled, unprocessed buffer anymore
	FilledBufferAvailable = false;  
	

	/////  End interaction with interrupt driven loop   /////

	//  Calculate parities for checking buffer
	calculateBufferParities();
	tmElements_t time;
	bool proccessedSucces;

	struct DCF77Buffer *rx_buffer;
	rx_buffer = (struct DCF77Buffer *)(unsigned long long)&processingBuffer;

	
	// Check parities
    if (flags.parityMin == rx_buffer->P1  &&
        flags.parityHour == rx_buffer->P2 &&
        flags.parityDate == rx_buffer->P3 &&
		rx_buffer->CEST != rx_buffer->CET) 
    { 
      //convert the received buffer into time	  	  	 
      time.Second = 0;
	  time.Minute = rx_buffer->Min-((rx_buffer->Min/16)*6);
      time.Hour   = rx_buffer->Hour-((rx_buffer->Hour/16)*6);
      time.Day    = rx_buffer->Day-((rx_buffer->Day/16)*6); 
      time.Month  = rx_buffer->Month-((rx_buffer->Month/16)*6);
      time.Year   = 2000 + rx_buffer->Year-((rx_buffer->Year/16)*6) -1970;
	  latestupdatedTime = makeTime(time);	 
	  CEST = rx_buffer->CEST;
	  //Parity correct
	  return true;
	} else {
	  //Parity incorrect
	  return false;
	}
}

/**
 * Calculate the parity of the time and date. 
 */
void DCF77::calculateBufferParities(void) {	
	// Calculate Parity 
	flags.parityFlag = 0;	
	for(int pos=0;pos<59;pos++) {
		bool s = (processingBuffer >> pos) & 1;  
		
		// Update the parity bits. First: Reset when minute, hour or date starts.
		if (pos ==  21 || pos ==  29 || pos ==  36) {
			flags.parityFlag = 0;
		}
		// save the parity when the corresponding segment ends
		if (pos ==  28) {flags.parityMin = flags.parityFlag;};
		if (pos ==  35) {flags.parityHour = flags.parityFlag;};
		if (pos ==  58) {flags.parityDate = flags.parityFlag;};
		// When we received a 1, toggle the parity flag
		if (s == 1) {
			flags.parityFlag = flags.parityFlag ^ 1;
		}
	}
}

/**
 * Store previous time. Needed for consistency 
 */
void DCF77::storePreviousTime(void) {
	previousUpdatedTime = latestupdatedTime;
	previousProcessingTimestamp = processingTimestamp;
}

void DCF77::checkTime(void*) {
	while(1) {
		if (!receivedTimeUpdate()) {
			currentTime = 0;
		} else {
			currentTime =latestupdatedTime + (now() - processingTimestamp);
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);	// delay for 1s
	}
}

/**
 * Get most recently received time 
 * Note, this only returns an time once, until the next update
 */
time_t DCF77::getTime(void)
{
	return currentTime;
}