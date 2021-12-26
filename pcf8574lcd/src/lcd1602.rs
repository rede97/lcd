use std::error::Error;
use std::io;
use std::io::{ErrorKind, Write};
use std::time::Duration;

pub trait LcdBus {
    type Error: Error;
    fn write(&mut self, cmd: bool, val: u8) -> Result<(), Self::Error>;
    fn read(&mut self, cmd: bool, val: u8) -> Result<u8, Self::Error>;
    fn backlight(&mut self, onoff: bool) -> Result<(), Self::Error>;
    fn sleep(&self, duration: Duration);
}

// commands
const LCD_CLEARDISPLAY: u8 = 0x01;
const LCD_RETURNHOME: u8 = 0x02;
const LCD_ENTRYMODESET: u8 = 0x04;
const LCD_DISPLAYCONTROL: u8 = 0x08;
const LCD_CURSORSHIFT: u8 = 0x10;
const LCD_FUNCTIONSET: u8 = 0x20;
const LCD_SETCGRAMADDR: u8 = 0x40;
const LCD_SETDDRAMADDR: u8 = 0x80;

// flags for display entry mode
const LCD_ENTRYRIGHT: u8 = 0x00;
const LCD_ENTRYLEFT: u8 = 0x02;
const LCD_ENTRYSHIFTINCREMENT: u8 = 0x01;
const LCD_ENTRYSHIFTDECREMENT: u8 = 0x00;

// flags for display on/off control
const LCD_DISPLAYON: u8 = 0x04;
const LCD_DISPLAYOFF: u8 = 0x00;
const LCD_CURSORON: u8 = 0x02;
const LCD_CURSOROFF: u8 = 0x00;
const LCD_BLINKON: u8 = 0x01;
const LCD_BLINKOFF: u8 = 0x00;

// flags for display/cursor shift
const LCD_DISPLAYMOVE: u8 = 0x08;
const LCD_CURSORMOVE: u8 = 0x00;
const LCD_MOVERIGHT: u8 = 0x04;
const LCD_MOVELEFT: u8 = 0x00;

// flags for function set
const LCD_8BITMODE: u8 = 0x10;
const LCD_4BITMODE: u8 = 0x00;
const LCD_2LINE: u8 = 0x08;
const LCD_1LINE: u8 = 0x00;
const LCD_5X10DOTS: u8 = 0x04;
const LCD_5X8DOTS: u8 = 0x00;

// flags for backlight control
const LCD_BACKLIGHT: u8 = 0x08;
const LCD_NOBACKLIGHT: u8 = 0x00;

pub struct Lcd1602<Bus: LcdBus> {
    bus: Bus,
    oled: bool,
    function: u8,
    control: u8,
    mode: u8,
}

impl<Bus> Lcd1602<Bus>
where
    Bus: LcdBus,
{
    pub fn new(bus: Bus, oled: bool) -> Lcd1602<Bus> {
        let function = LCD_4BITMODE | LCD_2LINE | LCD_5X8DOTS;
        return Lcd1602 {
            bus,
            oled,
            function,
            control: 0x00,
            mode: 0x00,
        };
    }

    fn send(&mut self, cmd: bool, val: u8) -> Result<(), Bus::Error> {
        self.bus.write(cmd, val & 0xf0)?;
        return self.bus.write(cmd, val << 4);
    }

    fn cmd(&mut self, cmd: u8) -> Result<(), Bus::Error> {
        return self.send(true, cmd);
    }

    fn write4(&mut self, cmd: u8) -> Result<(), Bus::Error> {
        return self.bus.write(true, cmd & 0xf0);
    }

    fn display(&mut self, onoff: bool) -> Result<(), Bus::Error> {
        if onoff {
            self.control |= LCD_DISPLAYON;
        } else {
            self.control &= !LCD_DISPLAYON;
        }
        return self.cmd(LCD_DISPLAYCONTROL | self.control);
    }

    pub fn set_cursor(&mut self, col: u8, row: u8) -> Result<(), Bus::Error> {
        let row_offset: [u8; 4] = [0x00, 0x40, 0x14, 0x54];
        let idx = row as usize % row_offset.len();
        return self.cmd(LCD_SETDDRAMADDR | (col + row_offset[idx]));
    }

    fn clear(&mut self) -> Result<(), Bus::Error> {
        self.cmd(LCD_CLEARDISPLAY)?;
        self.bus.sleep(Duration::from_micros(2000));
        if self.oled {
            self.set_cursor(0, 0)?;
        }
        return Ok(());
    }

    fn home(&mut self) -> Result<(), Bus::Error> {
        self.cmd(LCD_RETURNHOME)?;
        self.bus.sleep(Duration::from_micros(2000));
        return Ok(());
    }

    pub fn init(&mut self) -> Result<(), Bus::Error> {
        self.bus.backlight(true)?;
        self.bus.sleep(Duration::from_millis(100));
        // we start in 8bit mode, try to set 4 bit mode
        self.write4(0b11 << 4)?;
        self.bus.sleep(Duration::from_micros(4500));
        self.write4(0b11 << 4)?;
        self.bus.sleep(Duration::from_micros(4500));
        self.write4(0b11 << 4)?;
        self.bus.sleep(Duration::from_micros(150));
        // 4bit mode
        self.write4(0b10 << 4)?;
        self.cmd(LCD_FUNCTIONSET | self.function)?;
        self.control = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
        self.display(true)?;

        self.clear()?;

        self.mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
        self.cmd(LCD_ENTRYMODESET | self.mode)?;

        self.home()?;
        return Ok(());
    }
}

impl<Bus> Write for Lcd1602<Bus>
where
    Bus: LcdBus,
{
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        for c in buf {
            if (*c as u8) >> 4 == 0 {
                return Err(io::Error::new(ErrorKind::InvalidData, "unsupport char"));
            };
            self.send(false, *c).unwrap();
        }
        return Ok(buf.len());
    }
    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}
