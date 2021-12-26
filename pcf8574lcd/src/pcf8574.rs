use i2cdev::core::*;

pub struct PCF8574<Dev: I2CDevice> {
    dev: Dev,
    pin: u8,
}

impl<Dev> PCF8574<Dev>
where
    Dev: I2CDevice,
{
    pub fn new(mut dev: Dev, default_pin: Option<u8>) -> Result<PCF8574<Dev>, Dev::Error> {
        let pin = default_pin.or_else(|| Some(0)).unwrap();
        dev.smbus_write_byte(pin)?;
        return Ok(PCF8574 { dev, pin });
    }

    pub fn pin(&self) -> u8 {
        return self.pin;
    }

    pub fn set(&mut self, pin: u8) -> Result<(), Dev::Error> {
        self.pin = pin;
        self.dev.smbus_write_byte(pin)?;
        // println!("-> {:#010b}", pin);
        Ok(())
    }

    pub fn get(&mut self) -> Result<u8, Dev::Error> {
        return self.dev.smbus_read_byte();
    }
}
