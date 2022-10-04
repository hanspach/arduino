#ifndef DCF77_h
#define DCF77_h
#include <Arduino.h> 
#include <TimeLib.h>
#define DCFRejectionTime 700    // Pulse-to-Pulse rejection time. 
#define DCFRejectPulseWidth 50  // Minimal pulse width
#define DCFSplitTime 180        // Specifications distinguishes pulse width 100 ms and 200 ms. In practice we see 130 ms and 230
#define DCFSyncTime 1500        // Specifications defines 2000 ms pulse for end of sequence
#define MIN_TIME 1334102400     // Date: 11-4-2012
#define MAX_TIME 4102444800     // Date:  1-1-2100

class DCF77 {
    
    static int  dcfPin;
    static byte pulseStart;

    static time_t  previousUpdatedTime;  // DCF77 and internal timestamps
    static time_t  latestupdatedTime; 
    static time_t  currentTime;           
    static time_t  processingTimestamp;
    static time_t  previousProcessingTimestamp;     
    static unsigned char CEST;

    struct DCF77Buffer {                // DCF time format structure
      //unsigned long long prefix       :21;
        unsigned long long prefix     :17;
        unsigned long long CEST       :1; // CEST 
        unsigned long long CET        :1; // CET 
        unsigned long long unused     :2; // unused bits
        unsigned long long Min        :7; // minutes
        unsigned long long P1         :1; // parity minutes
        unsigned long long Hour       :6; // hours
        unsigned long long P2         :1; // parity hours
        unsigned long long Day        :6; // day
        unsigned long long Weekday    :3; // day of week
        unsigned long long Month      :5; // month
        unsigned long long Year       :8; // year (5 -> 2005)
        unsigned long long P3         :1; // parity
    };

    struct ParityFlags {              // parity format structure
        unsigned char parityFlag    :1;
        unsigned char parityMin     :1;
        unsigned char parityHour    :1;
        unsigned char parityDate    :1;
    } static flags;

    static volatile bool FilledBufferAvailable;      // shared between
    static volatile unsigned long long filledBuffer; // interrupt +
    static volatile time_t filledTimestamp;          // main loop

    static int  bufferPosition;                      // DCF-Buffers
    static unsigned long long runningBuffer;
    static unsigned long long processingBuffer;

    static   int  leadingEdge;            // pulse flancs
    static   int  trailingEdge;
    static   int  PreviousLeadingEdge;
    static   bool Up;

    void static initialize(void);
    void static bufferinit(void);
    void static finalizeBuffer(void);
    static bool receivedTimeUpdate(void);
    void static storePreviousTime(void);
    void static calculateBufferParities(void);
    bool static processBuffer(void);
    void static appendSignal(unsigned char signal);
    static void checkTime(void*);
    static void isr();
public:
    static time_t getTime(void);
    static void Start(const int);
    static void Stop(void);
};
#endif