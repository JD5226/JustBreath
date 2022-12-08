#include <mbed.h>
#include <bme280.h>

#define SAMPLE_TIME (500'000)
#define BUFFER_SIZE (20)
#define THRESHOLD (2.5)

I2C i2c(PC_9, PA_8);            // i2c(SDA, SCL)
const int addr7bit = 0x77;      // 7-bit I2C address
const int addr8bit = 0x77 << 1; // 8-bit I2C address

SPI spi(PA_7, PA_6, PA_5); // mosi, miso, sclk
DigitalOut cs(PA_4);

volatile float curr_temp = 0.0;
volatile float curr_humid = 0.0;
volatile float curr_pres = 0.0;
volatile bool start = false;
volatile bool alert = false;

float sample[20];     // circular buffer to collect the sampling data
int i = -1;           // circular buffer index
int counter = 0;      // the counter keeps track of the time elapsed, incrementing every 0.5 sec
float increase = 0.0; // the accumulated delta of an increasing period
float delta = 0.0;    // the delta of current sample and last sample

void user_delay_us(uint32_t period, void *intf_ptr)
{
  /*
   * Return control or wait,
   * for a period amount of milliseconds
   */
  // thread_sleep_for(period);
  wait_us(period);
}

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

/* Detection Algorithm */

// inhale: a decrease of humidity (-1.0)
// exhale: an increase of humidity (+2.5 ~ +4.0)
// stop: a contiguous decrease back to average or a constant average with slight flutuactions

// keep counting the time until there is an increase of >= 2.5
// (omitting every incease < 2.5, because it might be just some slight fluctations)
// restart counting whenever there is a decrease (no breathing condition)

/* timer and alert trigger */
void time_ticking()
{
  if (++counter >= 20) // increment the counter and see if greater than 20
  {
    // 20 means 10 seconds elapsed
    printf("stop breathing for 10s! \n");
    counter = 0;
    start = false;
  }
}

/* determine whether it is increasing */
bool on_increase(float data[])
{
  int j; // the index of the previous sample
  if (i > 0)
    j = i - 1;
  else
    j = 20 - 1;
  delta = data[i] - data[j];  // delta: current - previous
  return (data[i] > data[j]); // if current > previous, means increasing
}

/* find the exhalation, if there is an exhalation then reset the timer */
void breath_detection()
{
  /* fluctuation method -> not work!
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
    increase += delta; // calculate the total delta
    if (increase >= 2.5)
    {
      // when the total increasing comes to 2.5, means it is exhaling
      printf("-> exhaling...");
      counter = 0; // reset the counting because the person is breathing
    }
  }
  else
    increase = 0.0; // reset because it is decreasing

  printf("\n");
}

/* put humidity data into a circular buffer */
void humidity_collect(float data)
{
  if (++i >= 20)
    i = 0; // back to 0 when reaching the end -> circular

  sample[i] = data;
}

void print_sensor_data(struct bme280_data *comp_data)
{
#ifdef BME280_FLOAT_ENABLE
  printf("%0.2f, %0.2f, %0.2f\r\n", comp_data->temperature, comp_data->pressure, comp_data->humidity);
#else
  printf("%ld, %ld, %ld\r\n", comp_data->temperature, comp_data->pressure, comp_data->humidity);
#endif
}

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
  while (start && !alert)
  {
    /* Delay while the sensor completes a measurement */
    dev->delay_us(500'000, dev->intf_ptr); // measurement rate 0.5 sec

    rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, dev);
    // print_sensor_data(&comp_data);

    curr_temp = (float)comp_data.temperature;
    curr_humid = (float)comp_data.humidity;
    curr_pres = (float)comp_data.pressure;
    printf("time: %i |  humidity: %i  | temperature: %i  | pressure: %i ", counter, (int)curr_humid, (int)curr_temp, (int)curr_pres);

    humidity_collect(curr_humid);
    breath_detection();
    time_ticking();
  }

  return rslt;
}

/* SPI initialize */
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

  /* I2C initialize
  int8_t rslt = BME280_OK;
  uint8_t dev_addr = BME280_I2C_ADDR_SEC;

  dev.intf_ptr = &dev_addr;
  dev.intf = BME280_I2C_INTF;
  dev.read = user_i2c_read;
  dev.write = user_i2c_write;
  dev.delay_us = user_delay_ms;

  rslt = bme280_init(&dev);
  */

  bme280_dev_setup(&dev);

  if (bme280_init(&dev) == BME280_OK)
  {
    printf("BME280 initialize OK. \n");
    start = true;
  }

  while (1)
  {
    // put your main code here, to run repeatedly:
    stream_sensor_data_normal_mode(&dev);

    // auto-restart after every 10s
    // if breathing, start would never be set false
    while (!start)
    {
      printf("* end of a cycle * \n");
      printf("restart...\n");
      thread_sleep_for(3000);
      start = true;
    }
  }
}