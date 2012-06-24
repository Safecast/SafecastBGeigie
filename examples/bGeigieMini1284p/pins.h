
#ifndef __PIN_DEFINITION_H__
#define __PIN_DEFINITION_H__

// Main power switch
static const int main_switch = 2;
#define BG_MAIN_SWITCH_INTERRUPT 2;

// sensors
static const int batt_sense = A7;
static const int temp_sense = A6;
static const int hv_sense = A5;
static const int hum_sense = A4;
static const int sense_pwr = 3;

// Radio
static const int cs_radio = A3;
static const int radio_sleep = A2;
static const int irq_radio = 22;

// USB controller
static const int irq_32u4 = 23;
static const int cs_32u4 = 0;

// SD card
static const int cs_sd = 12;
static const int sd_detect = 20;
static const int sd_pwr = 14;

// GPS on/off
static const int gps_on_off = 21;

// Geiger counter
static const int counts = 1;
static const int hvps_pwr = 19;

// turn on and off high voltage power supply
#define bg_hvps_on() digitalWrite(hvps_pwr, LOW)
#define bg_hvps_off() digitalWrite(hvps_pwr, HIGH)

// LED on, off
static const int led = 13;
#define bg_led_on() digitalWrite(led, HIGH)
#define bg_led_off() digitalWrite(led, LOW)

#endif /* __PIN_DEFINITION_H__ */