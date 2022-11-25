#include <unity.h>
#include <blink.h>

void setUp(void)
{
  // set stuff up here
}

void tearDown(void)
{
  // clean stuff up here
}

void test_LEDon() {
    turnLEDon(LED_BUILTIN);
    TEST_ASSERT_EQUAL(1, digitalRead(LED_BUILTIN));
}

void test_LEDoff() {
    turnLEDoff(LED_BUILTIN);
    TEST_ASSERT_EQUAL(0, digitalRead(LED_BUILTIN));
}

void setup() {
    delay(2000); // Wait if board doesn't support software reset via Serial.DTR/RTS
    pinMode(LED_BUILTIN, OUTPUT);
    UNITY_BEGIN();
    RUN_TEST(test_LEDon);
    RUN_TEST(test_LEDoff);
    UNITY_END();   
}

void loop() {
}

