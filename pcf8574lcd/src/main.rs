mod lcd1602;
mod pcf8574;

use i2cdev::core::*;
use i2cdev::linux::LinuxI2CDevice;
use lcd1602::{Lcd1602, LcdBus};
use pcf8574::PCF8574;
use std::time::Duration;
use std::io::Write;

const DATA_MASK: u8 = 0xf0;
const LIGHT_PIN: u8 = 1 << 3;
const ENABLE_PIN: u8 = 1 << 2;
// const RW_PIN: u8 = 1 << 1; // 0: write 1: read
const RS_PIN: u8 = 1 << 0; // 0: cmd 1: data

struct PCF8574LcdBus<Dev: I2CDevice> {
    pcf8574: PCF8574<Dev>,
}

impl<Dev> PCF8574LcdBus<Dev>
where
    Dev: I2CDevice,
{
    fn new(dev: Dev) -> Result<PCF8574LcdBus<Dev>, Dev::Error> {
        let pcf8574 = PCF8574::<Dev>::new(dev, Some(0x00))?;
        return Ok(PCF8574LcdBus { pcf8574 });
    }
}

impl<Dev> LcdBus for PCF8574LcdBus<Dev>
where
    Dev: I2CDevice,
{
    type Error = Dev::Error;
    fn write(&mut self, cmd: bool, val: u8) -> Result<(), Dev::Error> {
        let mut pin = val & DATA_MASK | (self.pcf8574.pin() & LIGHT_PIN) | ENABLE_PIN;
        pin = match cmd {
            false => pin | RS_PIN,
            true => pin,
        };
        self.pcf8574.set(pin)?;
        pin = pin & (!ENABLE_PIN);
        self.sleep(Duration::from_micros(1));
        self.pcf8574.set(pin)?;
        self.sleep(Duration::from_micros(50));
        // println!("------------");
        return Ok(());
    }
    fn read(&mut self, _cmd: bool, _val: u8) -> Result<u8, Dev::Error> {
        return Ok(0x00);
    }
    fn backlight(&mut self, onoff: bool) -> Result<(), Dev::Error> {
        let pin = match onoff {
            true => self.pcf8574.pin() | LIGHT_PIN,
            false => self.pcf8574.pin() & (!LIGHT_PIN),
        };
        return self.pcf8574.set(pin);
    }

    fn sleep(&self, duration: Duration) {
        std::thread::sleep(duration);
    }
}

fn main() {
    let i2c_dev = "/dev/i2c-1";
    let dev = LinuxI2CDevice::new(i2c_dev, 0x27).expect(i2c_dev);
    let bus = PCF8574LcdBus::new(dev).unwrap();
    let mut lcd = Lcd1602::new(bus, true);
    lcd.init().unwrap();
    write!(lcd, "Rust: ");
    let mut cnt = 0;
    loop {
        lcd.set_cursor(6, 0).unwrap();
        write!(lcd, "{}", cnt).unwrap();
        cnt += 1;
        std::thread::sleep(Duration::from_secs(1));
    }
}
