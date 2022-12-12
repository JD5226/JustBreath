#include <mbed.h>
#include <bme280.h>

// System Parameters
#define SAMPLE_TIME (500'000)
#define BUFFER_SIZE (20)
#define THRESHOLD (2.5)

/* Digital Interface */
// I2C -> not work, deprecated later!
I2C i2c(PC_9, PA_8);            // i2c(SDA, SCL)
const int addr7bit = 0x77;      // 7-bit I2C address
const int addr8bit = 0x77 << 1; // 8-bit I2C address

// SPI wiring instruction:
// Connect Vin to 3V
// Connect GND to GND
// Connect SCK to PA5
// Connect SDO to PA6
// Connect SDI to PA7
// Connect CS to PA4
SPI spi(PA_7, PA_6, PA_5); // mosi, miso, sclk
DigitalOut cs(PA_4);

/* System State Codes */
// 0 - initialize
// 1 - running
// 2 - stop
// typical cycle: power on -> 0 -> 1 -> if alert -> 2 -> reset -> 1
// under construction...

/* Global Variables and Status Flags*/
volatile float curr_temp = 0.0;
volatile float curr_humid = 0.0;
volatile float curr_pres = 0.0;
volatile bool start = false;
volatile bool alert = false;

/* Variables for Data Processing */
float sample[20];     // circular buffer to collect the sampling data
int i = -1;           // circular buffer index
int counter = 0;      // the counter keeps track of the time elapsed, incrementing every 0.5 sec
float increase = 0.0; // the accumulated delta of an increasing period
float delta = 0.0;    // the delta of current sample and last sample
float IV = 0.0;
int icount = 0;

/* Variables for GUI */
// put your variables here...

void user_delay_us(uint32_t period, void *intf_ptr)
{
  /*
   * Return control or wait,
   * for a period amount of milliseconds
   */
  // thread_sleep_for(period);
  wait_us(period);
}

/* I2C read and write functions */
int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
  char data[len];

  /*
   * The parameter intf_ptr can be used as a variable to store the I2C address of the device
   */

  /*
   * Data on the bus should be like
   * |------------+---------------------|
   * | I2C action | Data                |
   * |------------+---------------------|
   * | Start      | -                   |
   * | Write      | (reg_addr)          |
   * | Stop       | -                   |
   * | Start      | -                   |
   * | Read       | (reg_data[0])       |
   * | Read       | (....)              |
   * | Read       | (reg_data[len - 1]) |
   * | Stop       | -                   |
   * |------------+---------------------|
   */
  i2c.start();
  data[0] = reg_addr;
  printf("i2c write: %i \n", i2c.write(addr8bit, data, len));
  i2c.stop();
  i2c.start();
  printf("i2c read: %i \n", i2c.read(addr8bit, data, len));
  i2c.stop();
  for (uint32_t i = 0; i < len; i++)
  {
    reg_data[i] = data[i];
  }

  // *reg_data = BME280_CHIP_ID;

  return rslt;
}

int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
  char data[len];

  /*
   * The parameter intf_ptr can be used as a variable to store the I2C address of the device
   */

  /*
   * Data on the bus should be like
   * |------------+---------------------|
   * | I2C action | Data                |
   * |------------+---------------------|
   * | Start      | -                   |
   * | Write      | (reg_addr)          |
   * | Write      | (reg_data[0])       |
   * | Write      | (....)              |
   * | Write      | (reg_data[len - 1]) |
   * | Stop       | -                   |
   * |------------+---------------------|
   */
  i2c.start();
  data[0] = reg_addr;
  i2c.write(addr8bit, data, 1);
  for (uint32_t i = 0; i < len; i++)
  {
    data[i] = reg_data[i];
    i2c.write(addr8bit, &data[i], 1);
  }
  i2c.stop();

  return rslt;
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

/* find max difference among values in array -> deprecated later! */
float find_max_diff(float arr[])
{
  // find the max diff of given arr
  // O(n^2) -> not good!
  float max_diff = 0.0;
  if (arr[1] - arr[0] >= 0)
    max_diff = arr[1] - arr[0];
  else
    max_diff = arr[0] - arr[1];

  for (int i = 0; i < 20; i++)
  {
    for (int j = i + 1; j < 20; j++)
    {
      float diff = 0.0;
      if (arr[j] - arr[i] >= 0)
        diff = arr[j] - arr[i];
      else
        diff = arr[i] - arr[j];

      if (diff > max_diff)
        max_diff = diff;
    }
  }
  return max_diff;
}

/* timer and alert trigger */
// Note:  this timer constantly runs in the background
//        for testing purpose, now we cycle the sytem for 10s sampling and wait for 3s to restart
//        if you want the system to act in different way, feel free to implement other timer functions!
void time_ticking()
{
  if (++counter >= 20) // increment the counter and see if greater than 20
  {
    // count to 20 means 10 seconds elapsed (under SAMPLE_TIME=0.5s)
    printf("stop breathing for 10s! \n");
    counter = 0;
    start = false;
    // alert = true;
  }
}

/* Detection Algorithm */

// 1 - Incresing Detection Method:
// inhale: a decrease of humidity (-1.0)
// exhale: an increase of humidity (+2.5 ~ +4.0)
// stop: a contiguous decrease back to average or a constant average with slight flutuactions

// keep counting the time until there is an increase of >= 2.5
// (omitting every incease < 2.5, because it might be just some slight fluctations)
// restart counting whenever there is a decrease (no breathing condition)

// 2 - Decreasing and No-Changing Detection Method:
// put your description here

/* determine the initial value of humidity */
float initialize(float data[])
{
  float sum = 0.0; 
  float avg = 0.0;
  for(int i = 0; i < 10; i++){
    sum += data[i];
    printf("Initializing.... \n");
  }
  avg = sum / 10;
  printf("Initialization Finialized. Initial Value: %d \n", (int)avg);
  return avg;
}

/* determine whether the value is increasing */
bool on_increase(float data[])
{
  int j; // the index of the previous sample
  if (i > 0)
    j = i - 1;
  else
    j = 20 - 1;
  delta = data[i] - data[j];  // delta: current - previous
  return (data[i] > data[j]); // if (current > previous) equals true, means increasing
}

/* determine whether the value is decreasing */
bool on_decrease(float data[]){
  int curr;
  int prev;
  if (i > 0){
    curr = data[i];
    prev = data[i - 1];
  }
  else{
    curr = data[i];
    prev = data[-1];
  }
  return curr < prev;
}

/* 1- find the exhalation, if there is an exhalation then reset the timer */
void breath_detection()
{
  /* fluctuation method -> not work, deprecated!
  // float fluctuation = find_max_diff(buffer);
  // if (fluctuation <= 5.0)
  //   counter++;
  // else
  //   counter = 0;

  // if (counter >= 20)
  // {
  //   // alert = true;
  //   counter = 0;
  // }
  */

  if (on_increase(sample)) // when increasing
  {
    increase += delta; // calculate the summation of delta
    if (increase >= 2.5)
    {
      // when the total increase comes to THRESHOLD, means the person is exhaling
      printf("-> exhaling...");
      counter = 0; // recount from 0 because the person is breathing at this moment
    }
  }
  else
    increase = 0.0; // reset because it is decreasing

  printf("\n");
}

/* 2- find the non-breathing period, if so, count the timer unitl 10 sec to trigger alert */
void no_breath_detection(float avg)
{
  // put your code here...
  if (on_decrease(sample) || (sample[i] < (avg + 2) && sample[i] > (avg - 2)))
      printf("-> not breathing...");
  else
    counter = 0; // reset because it is breathing
  printf("\n");
}

/* put humidity data into a circular buffer */
void humidity_collect(float data)
{
  if (++i >= 20)
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

/* GUI */
// put your code here...

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

  // printf("Temperature, Pressure, Humidity\r\n");

  /* the measurement cycle in normal mode */
  // put the data processing and detection algorithm functions in this loop
  while (start && !alert)
  {
    /* Delay while the sensor completes a measurement */
    dev->delay_us(500'000, dev->intf_ptr); // measurement rate 0.5 sec

    rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, dev);
    // print_sensor_data(&comp_data);

    curr_temp = (float)comp_data.temperature;
    curr_humid = (float)comp_data.humidity;
    curr_pres = (float)comp_data.pressure;

    // printf("time: %i |  humidity: %i  | temperature: %i  | pressure: %i ", counter, (int)curr_humid, (int)curr_temp, (int)curr_pres);
    // printf("time: %i |  humidity: %0.2f  | temperature: %0.2f  | pressure: %0.2f ", counter, curr_humid, curr_temp, curr_pres);

    humidity_collect(curr_humid);
    icount++;
    if (icount < 10){
      continue;
    }
    else if (icount == 10){
      IV = initialize(sample);
    }
      
    else{
      printf("time: %i |  humidity: %i  | temperature: %i  | pressure: %i ", counter, (int)curr_humid, (int)curr_temp, (int)curr_pres);
      no_breath_detection(IV);
      time_ticking();
    }
    
  }
  icount = 0;

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

int main()
{

  // put your setup code here, to run once:
  struct bme280_dev dev;

  bme280_dev_setup(&dev);

  if (bme280_init(&dev) == BME280_OK)
  {
    printf("BME280 initialize OK. \n");
    start = true;
  }

  /* the outer loop is for system state cycle */
  while (1)
  {
    /* 0- initialize */
    // run once, use normal mode to collect samples and find init_val
    // if(init_val==0)
    //   init_val = ...;

    /* 1- running */
    stream_sensor_data_normal_mode(&dev);

    /* 2- stop */
    // the inner loop is for "alert" or "periodically restart" purpose
    if (!start || alert)
    {
      printf("* end of a cycle * \n");

      // Note: auto-restart for testing purpose
      printf("restart...\n");
      thread_sleep_for(3000);
      start = true;
      alert = false;
    }
  }
}