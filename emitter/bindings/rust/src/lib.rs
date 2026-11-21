//! Thin FFI binding for the C SBOM emitter.
//!
//! SPDX-License-Identifier: MIT

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_longlong};
use std::ptr;

#[repr(C)]
pub struct SbomEmitter {
    _private: [u8; 0],
}

extern "C" {
    fn sbom_init(tool: *const c_char, version: *const c_char) -> *mut SbomEmitter;
    fn sbom_free(e: *mut SbomEmitter);
    fn sbom_add_node(
        e: *mut SbomEmitter,
        purl: *const c_char,
        name: *const c_char,
        version: *const c_char,
        supplier: *const c_char,
        license: *const c_char,
        hash: *const c_char,
    ) -> *const c_char;
    fn sbom_add_edge(e: *mut SbomEmitter, from: *const c_char, to: *const c_char) -> c_int;
    fn sbom_set_root(e: *mut SbomEmitter, id: *const c_char) -> c_int;
    fn sbom_set_author(
        e: *mut SbomEmitter,
        name: *const c_char,
        email: *const c_char,
        is_org: c_int,
    ) -> c_int;
    fn sbom_set_timestamp(e: *mut SbomEmitter, unix_seconds: c_longlong) -> c_int;
    fn sbom_write(e: *mut SbomEmitter, path: *const c_char) -> c_int;
    fn sbom_last_error(e: *const SbomEmitter) -> *const c_char;
}

fn cstr(s: Option<&str>) -> Option<CString> {
    s.filter(|s| !s.is_empty()).map(|s| CString::new(s).unwrap())
}

fn ptr_of(c: &Option<CString>) -> *const c_char {
    c.as_ref().map_or(ptr::null(), |c| c.as_ptr())
}

pub struct Emitter {
    handle: *mut SbomEmitter,
}

impl Emitter {
    pub fn new(tool: &str, version: &str) -> Result<Self, String> {
        let ct = CString::new(tool).unwrap();
        let cv = CString::new(version).unwrap();
        let handle = unsafe { sbom_init(ct.as_ptr(), cv.as_ptr()) };
        if handle.is_null() {
            Err("sbom_init failed".into())
        } else {
            Ok(Self { handle })
        }
    }

    fn error(&self) -> String {
        unsafe { CStr::from_ptr(sbom_last_error(self.handle)) }
            .to_string_lossy()
            .into_owned()
    }

    pub fn add_node(
        &mut self,
        purl: Option<&str>,
        name: Option<&str>,
        version: Option<&str>,
        supplier: Option<&str>,
        license: Option<&str>,
        hash: Option<&str>,
    ) -> Result<String, String> {
        let (cp, cn, cv) = (cstr(purl), cstr(name), cstr(version));
        let (cs, cl, ch) = (cstr(supplier), cstr(license), cstr(hash));
        let id = unsafe {
            sbom_add_node(
                self.handle,
                ptr_of(&cp),
                ptr_of(&cn),
                ptr_of(&cv),
                ptr_of(&cs),
                ptr_of(&cl),
                ptr_of(&ch),
            )
        };
        if id.is_null() {
            Err(self.error())
        } else {
            Ok(unsafe { CStr::from_ptr(id) }.to_string_lossy().into_owned())
        }
    }

    pub fn add_edge(&mut self, from: &str, to: &str) -> Result<(), String> {
        let (cf, ct) = (CString::new(from).unwrap(), CString::new(to).unwrap());
        if unsafe { sbom_add_edge(self.handle, cf.as_ptr(), ct.as_ptr()) } != 0 {
            Err(self.error())
        } else {
            Ok(())
        }
    }

    pub fn set_root(&mut self, id: &str) -> Result<(), String> {
        let ci = CString::new(id).unwrap();
        if unsafe { sbom_set_root(self.handle, ci.as_ptr()) } != 0 {
            Err(self.error())
        } else {
            Ok(())
        }
    }

    pub fn set_author(&mut self, name: &str, email: Option<&str>, is_org: bool) -> Result<(), String> {
        let cn = CString::new(name).unwrap();
        let ce = cstr(email);
        let rc = unsafe {
            sbom_set_author(self.handle, cn.as_ptr(), ptr_of(&ce), if is_org { 1 } else { 0 })
        };
        if rc != 0 {
            Err(self.error())
        } else {
            Ok(())
        }
    }

    pub fn set_timestamp(&mut self, unix_seconds: i64) -> Result<(), String> {
        if unsafe { sbom_set_timestamp(self.handle, unix_seconds as c_longlong) } != 0 {
            Err(self.error())
        } else {
            Ok(())
        }
    }

    pub fn write(&self, path: &str) -> Result<(), String> {
        let cp = CString::new(path).unwrap();
        if unsafe { sbom_write(self.handle, cp.as_ptr()) } != 0 {
            Err(self.error())
        } else {
            Ok(())
        }
    }
}

impl Drop for Emitter {
    fn drop(&mut self) {
        unsafe { sbom_free(self.handle) }
    }
}
