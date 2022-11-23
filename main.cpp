#include "mbed.h"
#include "BME.h"


BME sensor(PB_7,PB_6, 0x76); // sda, clk, 8bit address
DigitalOut led(LED1);

float temp, humidity, pressure, altitude;

int main()
{

    while(1) {
        led=1;
        printf("\033[0m\033[2J\033[H------- BME280-BMP280 Sensor example -------\r\n\n");

        int BMPE_id = sensor.init();    // initialise and get sensor id

        if(!BMPE_id) {
            printf("No sensor detected!!\n");
            led=0;
            ThisThread::sleep_for(200);
        } else {

            if(BMPE_id==0x60) {
                printf("BME280 detected id: 0x%x\n\n",BMPE_id);
            } else if(BMPE_id==0x58) {
                printf("BMP280 detected id: 0x%x\n\n",BMPE_id);
            }

            temp = sensor.getTemperature();
            printf("Temperature: %3.2f %cc \n",temp,0xb0);

            if(sensor.chip_id==0x60) {        // only BME has Humidity
                humidity    = sensor.getHumidity();
                printf("Humidity:    %2.2f %cRh \n", humidity,0x25);
            }
            pressure    = sensor.getPressure();
            printf("Pressure:    %4.2f mbar's \n\n",pressure);

            altitude = 44330.0f*( 1.0f - pow((pressure/1013.25f), (1.0f/5.255f)))+18;     // Calculate altitude in meters
            printf("Altitude: %3.1f meters   %4.1f feet \r\n(Referenced to 1,013.25 millibars @ sea level) \r\n\n", altitude,altitude*3.2810f);
            led=0;
            ThisThread::sleep_for(1000);
        }
    }
}