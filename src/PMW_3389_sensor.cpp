#include <Arduino.h>
#include <SPI.h>

#include "PMW_3389_sensor.h"
#include "srom_3389.h"

// Writes to a register on the PMW3389
void write_reg_PMW(uint8_t reg, uint8_t value)
{
  reg               |= 0b10000000;
  uint16_t transfer  = (reg << 8) | value;

  ACTIVATE_CS_PMW;
  SPI.transfer16(transfer);
  DEACTIVATE_CS_PMW;

  delayMicroseconds(TSWW);
}

// Reads out a register on the PMW3389
uint8_t read_reg_PMW(uint8_t reg)
{
  reg &= 0b01111111;

  ACTIVATE_CS_PMW;
  SPI.transfer     (reg);
  delayMicroseconds(TSRAD);
  uint8_t value = SPI.transfer(0);
  DEACTIVATE_CS_PMW;

  delayMicroseconds(TSRR);

  return value;
}

// Helper function for burst load uploads 1 byte to a preset address
void upload_byte(uint8_t value)
{
  SPI.transfer     (value);
  delayMicroseconds(TLOAD);
}

// Uploads a binary firmware file over SPI to the PMW3389
void upload_firmware()
{
  // Prepare to upload the binary file
  write_reg_PMW(REG_Config2, 0x00);
  write_reg_PMW(REG_SROM_Enable, 0x1d);
  delay        (10);                    // From datasheet
  write_reg_PMW(REG_SROM_Enable, 0x18);

  // Upload the binary file here
  ACTIVATE_CS_PMW;
  upload_byte(REG_SROM_Load_Burst | 0x80);
  for        (uint32_t index = 0; index < 4094; index++) upload_byte(firmware_data[index]);
  DEACTIVATE_CS_PMW;

  read_reg_PMW (REG_SROM_ID);
  write_reg_PMW(REG_Config2, 0x00);
}

// Initializes the PMW3389
uint32_t begin_PMW()
{
  pinMode(PMW_CS   , OUTPUT);
  pinMode(PMW_RESET, OUTPUT);

  SPI.begin();
  RESET_SPI;
  RESET_PMW;
  delay    (50);

  SPI.beginTransaction(SPI_SETTINGS_PMW);
  Serial.println      ("Starting firmware upload");
  upload_firmware     ();
  SPI.endTransaction  ();
  delay               (10);

  return check_signature();
}

// Checks if the firmware was correctly uploaded
uint32_t check_signature()
{
  SPI.beginTransaction(SPI_SETTINGS_PMW);

  uint8_t pid      = read_reg_PMW(REG_Product_ID);
  uint8_t iv_pid   = read_reg_PMW(REG_Inverse_Product_ID);
  uint8_t SROM_ver = read_reg_PMW(REG_SROM_ID);

  SPI.endTransaction();

  return (pid == 0x47 && iv_pid == 0xB8 && SROM_ver == 0xE8); // signature for PMW33389
}

// Sets the CPI/DPI value of the PMW3389
void set_CPI(uint16_t cpi)
{
  // Limits cpi to 100 - 12000 effectivly with steps of a 100
  cpi = constrain((cpi / 100) - 1, 0, 119);
  write_reg_PMW  (REG_Config1,     cpi);
}
