use nix::fcntl::{open, OFlag};
use nix::ioctl_read;
use nix::ioctl_write_int;
use nix::sys::ioctl;
use nix::sys::stat::Mode;
use nix::unistd::{close, write};

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

fn main() {
    let lcd =
        open("/dev/pcflcd", OFlag::O_RDWR | OFlag::O_SYNC, Mode::empty()).expect("open sha256 dev");
    unsafe {
        lcd_clear(lcd, 0).unwrap();
        lcd_set_blink(lcd, 1).unwrap();
    }
    write(lcd, "Hello".as_bytes()).unwrap();
    let mut lcd_size: u32 = 0;
    unsafe {
        lcd_get_size(lcd, &mut lcd_size).unwrap();
        let pos = mk_pos(1, 8);
        println!("{:x}", pos);
        lcd_set_cursor_pos(lcd, pos).unwrap();
    }

    println!(
        "lcd row: {} col: {}",
        (lcd_size >> 8) & 0xff,
        lcd_size & 0xff
    );

    write(lcd, "Rust!".as_bytes()).unwrap();

    close(lcd).unwrap();
}
