use nix::fcntl::{open, OFlag};
use nix::ioctl_read;
use nix::ioctl_write_int;
use nix::sys::ioctl;
use nix::sys::stat::Mode;
use nix::unistd::{close, write};
use std::io::{Result, Write};
use std::time::Duration;

const PCFLCD_MAGIC: u8 = b'P';
const PCFLCD_GET_SIZE: u8 = 0;
const PCFLCD_GET_OLED: u8 = 1;
const PCFLCD_SET_BACKLIGHT: u8 = 2;
const PCFLCD_SET_DISPLAY: u8 = 3;
const PCFLCD_SET_CURSOR: u8 = 4;
const PCFLCD_SET_BLINK: u8 = 5;
const PCFLCD_SET_SCROLL: u8 = 6;
const PCFLCD_SET_ENTRY: u8 = 7;
const PCFLCD_SET_AUTO_SCROLL: u8 = 8;
const PCFLCD_SET_CURSOR_POS: u8 = 9;
const PCFLCD_CLEAR: u8 = 10;

ioctl_read!(lcd_get_size, PCFLCD_MAGIC, PCFLCD_GET_SIZE, u32);
ioctl_write_int!(lcd_set_cursor_pos, PCFLCD_MAGIC, PCFLCD_SET_CURSOR_POS);
ioctl_write_int!(lcd_clear, PCFLCD_MAGIC, PCFLCD_CLEAR);
ioctl_write_int!(lcd_set_blink, PCFLCD_MAGIC, PCFLCD_SET_BLINK);

fn mk_pos(row: u8, col: u8) -> u32 {
    return (row as u32) << 8 | (col as u32);
}

struct LCD {
    fd: i32,
}

impl LCD {
    fn new(fd: i32) -> LCD {
        return LCD { fd };
    }
}

impl Write for LCD {
    fn write(&mut self, buf: &[u8]) -> Result<usize> {
        let n = write(self.fd, buf)?;
        return Ok(n);
    }

    fn flush(&mut self) -> Result<()> {
        Ok(())
    }
}

fn main() {
    let lcdfd =
        open("/dev/usblcd", OFlag::O_RDWR | OFlag::O_SYNC, Mode::empty()).expect("open sha256 dev");
    let mut lcd_size: u32 = 0;

    unsafe {
        lcd_get_size(lcdfd, &mut lcd_size).unwrap();

        println!(
            "lcd row: {} col: {}",
            (lcd_size >> 8) & 0xff,
            lcd_size & 0xff
        );
    }

    unsafe {
        lcd_clear(lcdfd, 0).unwrap();
        //lcd_set_blink(lcdfd, 1).unwrap();
    }

    let mut lcd = LCD::new(lcdfd);

    write!(lcd, "2021-12-26 128th");
    let mut cnt = 0;
    let pos = mk_pos(1, 2);
    unsafe {
        lcd_set_cursor_pos(lcdfd, pos as u64).unwrap();
    }
    write!(lcd, "\0Anniversary\0");
    loop {
        // unsafe {
        //     lcd_set_cursor_pos(lcdfd, pos as u64).unwrap();
        // }
        // write!(lcd, "{}", cnt).unwrap();
        cnt += 1;
        std::thread::sleep(Duration::from_secs(1));
    }

    close(lcdfd).unwrap();
}
