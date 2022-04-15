#include "Heater_Software.h"
uint8_t heater_overheat = 0;

void setup()
{
    // inputs to MOSFET circuits
    pinMode(TOGGLE_PINS[0], OUTPUT);
    pinMode(TOGGLE_PINS[1], OUTPUT);
    pinMode(TOGGLE_PINS[2], OUTPUT);

    // data from temperature sensors
    pinMode(THERMO_DATA_1, INPUT);
    pinMode(THERMO_DATA_2, INPUT);
    pinMode(THERMO_DATA_3, INPUT);

    // sets overheat LED pins
    pinMode(HEATER_OVERHEAT_LEDS[0], OUTPUT);
    pinMode(HEATER_OVERHEAT_LEDS[1], OUTPUT);
    pinMode(HEATER_OVERHEAT_LEDS[2], OUTPUT);

    Serial.begin(115200);

    // Set up rovecomm with the correct IP and the TCP server
    RoveComm.begin(RC_HEATERBOARD_FOURTHOCTET, &TCPServer);
    delay(100);

    Serial.println("Started: ");

    // update timekeeping
    last_update_time = millis();
}

void loop()
{
    packet = RoveComm.read();
    // monitoring temperature values and toggling individual heaters
    switch (packet.data_id)
    {
    case RC_HEATERBOARD_HEATERTOGGLE_DATA_ID:
        heater_enabled = (uint8_t)packet.data[0];

        for (uint8_t i = 0; i < 3; i++)
        {
            if (heater_enabled & (1 << i))
            {
                digitalWrite(TOGGLE_PINS[i], HIGH);
                Serial.println("Enabled");
            }

            else
            {
                if (!(heater_enabled & (1 << i)))
                {
                    digitalWrite(TOGGLE_PINS[i], LOW);
                }
            }
        }
        break;
    }

    // temperature data from each sensor
    float temp1 = analogRead(THERMO_DATA_1);
    float temp2 = analogRead(THERMO_DATA_2);
    float temp3 = analogRead(THERMO_DATA_3);

    // changes ADC values from temperature sensors to Celsius
    float temp1Celsius = map(temp1, TEMP_ADC_MIN, TEMP_ADC_MAX, TEMP_MIN, TEMP_MAX) / 1000;
    float temp2Celsius = map(temp2, TEMP_ADC_MIN, TEMP_ADC_MAX, TEMP_MIN, TEMP_MAX) / 1000;
    float temp3Celsius = map(temp3, TEMP_ADC_MIN, TEMP_ADC_MAX, TEMP_MIN, TEMP_MAX) / 1000;

    float temps[3] = {temp1Celsius, temp2Celsius, temp3Celsius};

    for (uint8_t i = 0; i < 3; i++)
    {
        if (temps[i] >= 105 || !(heater_enabled & (1 << i)))
        {
            digitalWrite(TOGGLE_PINS[i], LOW);

            if (temps[i] >= 115)
            {
                heater_overheat |= 1 << i; // OR the overheat var with 1 to make it 1
                Serial.println("over");
                digitalWrite(HEATER_OVERHEAT_LEDS[i], HIGH);
            }
        }

        if (temps[i] <= 105)
        {
            heater_overheat &= !(1 << i);
            // if a heater is overheated, its value in the bit array is 1
            // !(1 << i) sets every 0 in the bit arrary to 1 and every one to 0
            // if heater_overheat == 1, !(1 << i) yields a 0 and AND with 0 always yields 0
            // if heater_overheat == 0, the AND will yield 0, turning off the overheat variable
            digitalWrite(HEATER_OVERHEAT_LEDS[i], LOW);
        }
    }

    // sends data to basestation
    if (millis() - last_update_time >= ROVECOMM_UPDATE_RATE)
    {
        if (heater_overheat)
        {
            RoveComm.write(RC_HEATERBOARD_OVERHEAT_DATA_ID, RC_HEATERBOARD_OVERHEAT_DATA_COUNT, heater_overheat);
        }

        RoveComm.write(RC_HEATERBOARD_HEATERENABLED_DATA_ID, RC_HEATERBOARD_HEATERENABLED_DATA_COUNT, heater_enabled);
        RoveComm.write(RC_HEATERBOARD_THERMOVALUES_DATA_ID, RC_HEATERBOARD_THERMOVALUES_DATA_COUNT, temps);
        last_update_time = millis();
    }
}
