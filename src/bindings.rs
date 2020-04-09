#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

#[cfg(test)]
mod tests {
    use super::*;

    fn print<T: Into<String>>(s: T) {
        use std::io::Write;

        let stdout = std::io::stdout();
        let mut stdout = stdout.lock();
        writeln!(stdout, "{}", s.into()).unwrap();
    }

    extern "C" fn on_paint_callback(
        new_pixels: *const ::std::os::raw::c_void,
        new_width: ::std::os::raw::c_int,
        new_height: ::std::os::raw::c_int,
    ) {
        print(format!(
            "paint {} {} {:?}",
            new_width, new_height, new_pixels
        ));
    }

    #[ignore]
    #[test]
    fn it_works() {
        unsafe {
            assert_eq!(cef_init(Some(on_paint_callback)), 0);

            fn run_script(code: String) {
                use std::ffi::CString;
                let c_str = CString::new(code).unwrap();
                unsafe {
                    assert_eq!(cef_run_script(c_str.as_ptr()), 0);
                }
            }

            for i in 0..200 {
                if i == 50 {
                    run_script(format!("player.loadVideoById(\"{}\");", "gQngg8iQipk"));
                }
                assert_eq!(cef_step(), 0);
                std::thread::sleep(std::time::Duration::from_millis(20));
            }

            assert_eq!(cef_free(), 0);
        }
    }
}