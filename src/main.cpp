/**
 * @file main.cpp
 * @author Roberto Hong (zh2441), Eugene Lan (yl8241), Dennis Li (xl4141), Jiaxin Dong (jd5226)
 * @brief Embedded Challenge Fall 2022 Team Project: Just Breathe
 * @version 0.1
 * @date 2022-12-15
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <mbed.h>
#include <bme280.h>
#include "drivers/LCD_DISCO_F429ZI.h"

// System Parameters
#define SAMPLE_TIME (500'000) // in microsecond
#define BUFFER_SIZE (20)

// UI System Parameters
#define BACKGROUND 1
#define FOREGROUND 0
#define GRAPH_PADDING 5

/* Variables for GUI */
LCD_DISCO_F429ZI lcd;
char display_buf[3][60];

/* Digital Interface (SPI)*/
// Wiring instruction: (Vin - 3V), (GND - GND), (SCK - PA5), (SDO - PA6), (SDI - PA7), (CS - PA4)
SPI spi(PA_7, PA_6, PA_5); // mosi, miso, sclk
DigitalOut cs(PA_4);

/* Global Variables and Status Flags*/
volatile float curr_temp = 0.0;
volatile float curr_humid = 0.0;
volatile float curr_pres = 0.0;
volatile bool start = false;
volatile bool alert = false;

/* Variables for Data Processing */
float sample[BUFFER_SIZE]; // circular buffer to collect the sampling data
int i = -1;                // circular buffer index
int counter = 0;           // counter to keep track of the time elapsed, increments every 0.5 sec
float IV = 0.0;            // initial value of humidity
int icount = 0;            // initialization counter

void setupScreen()
{
  // background
  lcd.SelectLayer(BACKGROUND);
  lcd.Clear(LCD_COLOR_BLACK);
  lcd.SetBackColor(LCD_COLOR_BLACK);
  lcd.SetTextColor(LCD_COLOR_YELLOW);
  lcd.SetLayerVisible(BACKGROUND, ENABLE);
  lcd.SetTransparency(BACKGROUND, 0x7Fu);
  // forground
  lcd.SelectLayer(FOREGROUND);
  lcd.Clear(LCD_COLOR_BLACK);
  lcd.SetBackColor(LCD_COLOR_BLACK);
  lcd.SetTextColor(LCD_COLOR_LIGHTGREEN);
}

void displayValue(uint8_t temp, uint8_t moi, uint8_t pres, bool initializing = false)
{
  // if initializing
  if (initializing)
  {
    snprintf(display_buf[0], 60, "Initializing...");
    lcd.SelectLayer(BACKGROUND);
    lcd.Clear(LCD_COLOR_BLACK);
    lcd.DisplayStringAt(0, LINE(10), (uint8_t *)display_buf[0], CENTER_MODE);
    return;
  }

  // if alert, set screen to red and print the alert message
  if (alert)
  {
    snprintf(display_buf[0], 60, "SIDS Alert !");
    lcd.SelectLayer(BACKGROUND);
    lcd.Clear(LCD_COLOR_RED);
    lcd.SetBackColor(LCD_COLOR_RED);
    lcd.DisplayStringAt(0, LINE(10), (uint8_t *)display_buf[0], CENTER_MODE);
  }
  else
  {
    lcd.SelectLayer(BACKGROUND);
    lcd.Clear(LCD_COLOR_BLACK);
    lcd.SetBackColor(LCD_COLOR_BLACK);
  }
  // update value in buffer
  snprintf(display_buf[1], 60, "Humidity: %d ", moi);
  snprintf(display_buf[0], 60, "Temperature: %d ", temp);
  snprintf(display_buf[2], 60, "Pressure: %d ", pres);
  lcd.SelectLayer(FOREGROUND);
  // display
  lcd.DisplayStringAt(0, LINE(2), (uint8_t *)display_buf[0], LEFT_MODE);
  lcd.DisplayStringAt(0, LINE(1), (uint8_t *)display_buf[1], LEFT_MODE);
  lcd.DisplayStringAt(0, LINE(3), (uint8_t *)display_buf[2], LEFT_MODE);
}

/* SPI device delay */
void user_delay_us(uint32_t period, void *intf_ptr)
{
  /*
   * Return control or wait,
   * for a period amount of microseconds
   */
  wait_us(period);
}

/* SPI read and write functions */
int8_t user_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

  /*
   * The parameter intf_ptr can be used as a variable to select which Chip Select pin has
   * to be set low to activate the relevant device on the SPI bus
   */

  /*
   * Data on the bus should be like
   * |----------------+---------------------+-------------|
   * | MOSI           | MISO                | Chip Select |
   * |----------------+---------------------|-------------|
   * | (don't care)   | (don't care)        | HIGH        |
   * | (reg_addr)     | (don't care)        | LOW         |
   * | (don't care)   | (reg_data[0])       | LOW         |
   * | (....)         | (....)              | LOW         |
   * | (don't care)   | (reg_data[len - 1]) | LOW         |
   * | (don't care)   | (don't care)        | HIGH        |
   * |----------------+---------------------|-------------|
   */

  cs = 1;
  spi.format(8, 3);
  spi.frequency(1000000);

  cs = 0;
  spi.write(reg_addr);

  int data[len];
  for (uint32_t i = 0; i < len; i++)
  {
    data[i] = spi.write(0x00);
  }

  // printf("read from %X: 0x%X\n", reg_addr, data[0]);
  for (uint32_t i = 0; i < len; i++)
  {
    reg_data[i] = data[i];
  }

  cs = 1;

  return rslt;
}

int8_t user_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

  /*
   * The parameter intf_ptr can be used as a variable to select which Chip Select pin has
   * to be set low to activate the relevant device on the SPI bus
   */

  /*
   * Data on the bus should be like
   * |---------------------+--------------+-------------|
   * | MOSI                | MISO         | Chip Select |
   * |---------------------+--------------|-------------|
   * | (don't care)        | (don't care) | HIGH        |
   * | (reg_addr)          | (don't care) | LOW         |
   * | (reg_data[0])       | (don't care) | LOW         |
   * | (....)              | (....)       | LOW         |
   * | (reg_data[len - 1]) | (don't care) | LOW         |
   * | (don't care)        | (don't care) | HIGH        |
   * |---------------------+--------------|-------------|
   */
  cs = 1;
  spi.format(8, 3);
  spi.frequency(1000000);

  cs = 0;
  spi.write(reg_addr);
  for (uint32_t i = 0; i < len; i++)
  {
    spi.write(reg_data[i]);
    // printf("write: 0x%X\n", reg_data[i]);
  }
  cs = 1;

  return rslt;
}

/* timer and alert trigger */
// this timer constantly runs in the background after initialization
void time_ticking()
{
  if (++counter >= 20) // increment the counter and see if greater than 20
  {
    // count to 20 means 10 seconds elapsed (under SAMPLE_TIME=0.5s)
    printf("stop breathing for 10s! \n");
    counter = 0;
    icount = 0;
    start = false;
    alert = true;
  }
}

/*
Detection Algorithm

inhale: a decrease of humidity (-1.0)
exhale: an increase of humidity (+2.5 ~ +4.0)
stop: a contiguous decrease back to average or a constant average with slight flutuactions

1 - Incresing Detection Method: (deprecated)
keep counting the time until there is an increase of >= 2.5
(omitting every incease < 2.5, because it might be just some slight fluctations)
restart counting whenever there is a decrease (no breathing condition)

2 - Decreasing and No-Changing Detection Method:
if the humidity remains the same (+-3 from initial value) or the humidity is constantly decreasing
means the person is not breathing, start counting to 10 sec

refer to README.md for further details

*/

/* determine the initial value of humidity */
// initial value: the humidity when the person is not breathing
float initialize(float data[])
{
  float sum = 0.0;
  float avg = 0.0;
  for (int i = 0; i < 10; i++)
  {
    sum += data[i];
    printf("Initializing.... \n");
  }
  avg = sum / 10;
  printf("Initialization Finalized. Initial Value: %d \n", (int)avg);
  return avg;
}

/* determine whether the value is decreasing */
bool on_decrease(float data[])
{
  int curr;
  int prev;
  if (i > 0)
  {
    curr = data[i];
    prev = data[i - 1];
  }
  else
  {
    curr = data[i];
    prev = data[-1];
  }
  return curr <= prev;
}

/* 2- find the non-breathing period */
void no_breath_detection()
{
  if (on_decrease(sample) || (sample[i] < (IV + 3) && sample[i] > (IV - 3)))
    printf("-> not breathing...");
  else
    counter = 0; // reset because it is breathing
  printf("\n");
}

/* put humidity data into a circular buffer */
void humidity_collect(float data)
{
  if (++i >= BUFFER_SIZE)
    i = 0; // back to 0 when reaching the end -> circular

  sample[i] = data; // store the data to the buffer
}

/* BME280 data streaming and printing functions */
// for further details, refer to https://github.com/BoschSensortec/BME280_driver
void print_sensor_data(struct bme280_data *comp_data)
{
#ifdef BME280_FLOAT_ENABLE
  printf("%0.2f, %0.2f, %0.2f\r\n", comp_data->temperature, comp_data->pressure, comp_data->humidity);
#else
  printf("%ld, %ld, %ld\r\n", comp_data->temperature, comp_data->pressure, comp_data->humidity);
#endif
}

/* stream the data read from sensor */
int8_t stream_sensor_data_normal_mode(struct bme280_dev *dev)
{
  int8_t rslt;
  uint8_t settings_sel;
  struct bme280_data comp_data;

  /* Recommended mode of operation: Indoor navigation */
  dev->settings.osr_h = BME280_OVERSAMPLING_1X;
  dev->settings.osr_p = BME280_OVERSAMPLING_16X;
  dev->settings.osr_t = BME280_OVERSAMPLING_2X;
  dev->settings.filter = BME280_FILTER_COEFF_OFF;
  dev->settings.standby_time = BME280_STANDBY_TIME_62_5_MS;

  settings_sel = BME280_OSR_PRESS_SEL;
  settings_sel |= BME280_OSR_TEMP_SEL;
  settings_sel |= BME280_OSR_HUM_SEL;
  settings_sel |= BME280_STANDBY_SEL;
  settings_sel |= BME280_FILTER_SEL;
  rslt = bme280_set_sensor_settings(settings_sel, dev);
  rslt = bme280_set_sensor_mode(BME280_NORMAL_MODE, dev);

  /* the measurement cycle in normal mode */
  while (start && !alert)
  {
    /* Delay while the sensor completes a measurement */
    dev->delay_us(SAMPLE_TIME, dev->intf_ptr); // measurement rate 0.5 sec

    rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, dev);

    curr_temp = (float)comp_data.temperature;
    curr_humid = (float)comp_data.humidity;
    curr_pres = (float)comp_data.pressure;

    humidity_collect(curr_humid);
    icount++;
    if (icount < 10)
    {

      displayValue(0, 0, 0, true); // initializing
      thread_sleep_for(1000);
      continue;
    }
    else if (icount == 10)
    {
      IV = initialize(sample); // use the data from first 10 seconds to calculate initial value
    }
    else // detection running: show the data and call the algo to determine breathing or not
    {
      printf("time: %i |  humidity: %i  | temperature: %i  | pressure: %i ", counter, (int)curr_humid, (int)curr_temp, (int)curr_pres);
      displayValue((int)curr_temp, (int)curr_humid, (int)curr_pres);
      no_breath_detection();
      time_ticking();
    }
  }

  return rslt;
}

/* SPI setup */
void bme280_dev_setup(struct bme280_dev *dev)
{
  uint8_t dev_addr = 0; // Sensor_0 interface over SPI with native chip select line
  dev->intf_ptr = &dev_addr;
  dev->intf = BME280_SPI_INTF;
  dev->read = user_spi_read;
  dev->write = user_spi_write;
  dev->delay_us = user_delay_us;
}

/*
 * System State:
 * 0 - initialize
 * 1 - running
 * 2 - stop
 * typical cycle: power on -> 0 -> 1 -> if alert -> 2 -> reset -> 0 -> 1
 */
int main()
{
  // setup the sensor and the screen
  struct bme280_dev dev;
  bme280_dev_setup(&dev);
  if (bme280_init(&dev) == BME280_OK)
  {
    printf("BME280 initialize OK. \n");
    start = true;

    setupScreen();
  }

  /* system state cycle */
  while (1)
  {

    /* 0- initialize and 1- running */
    stream_sensor_data_normal_mode(&dev);

    /* 2- stop */
    if (!start || alert)
    {
      printf("* end of a cycle * \n");
      displayValue((int)curr_temp, (int)curr_humid, (int)curr_pres);
      break;
    }
  }
}