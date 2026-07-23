use serde::Serialize;
use std::{
    fs,
    ffi::{c_char, c_void, CStr, CString},
    path::PathBuf,
};
use tauri::{Manager, State};
use tauri::ipc::Channel;

unsafe extern "C" {
    fn sq1_unkarnify(input: *const std::ffi::c_char, output: *mut std::ffi::c_char, capacity: usize) -> i32;
    fn sq1_karnify(input: *const std::ffi::c_char, output: *mut std::ffi::c_char, capacity: usize) -> i32;
    fn sq1_karnify_smart(input: *const std::ffi::c_char, position: *const std::ffi::c_char, generator: bool,
                         output: *mut std::ffi::c_char, capacity: usize) -> i32;
    fn sq1_run_alloc(argc: i32, argv: *const *const c_char, table_directory: *const c_char, exit_code: *mut i32,
                     callback: extern "C" fn(*const c_char, *mut c_void), callback_context: *mut c_void) -> *mut c_char;
    fn sq1_free_string(value: *mut c_char);
    fn sq1_request_stop();
    fn sq1_rate_algorithm(algorithm: *const c_char, initial_top_a: bool, output: *mut RatingResult) -> bool;
    fn sq1_two_gen_compatibility(position: *const i32, corners_two: *mut bool, corners_pseudo: *mut bool) -> i32;
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Serialize)]
#[serde(rename_all = "camelCase")]
struct RatingResult {
    final_score: f64,
    phase1: f64,
    phase2: f64,
    phase3: f64,
    phase4: f64,
    ergo_up: f64,
    ergo_down: f64,
    slice_count: i32,
    movement: i32,
    bonus: i32,
    valid: bool,
    slice_start: i8,
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
struct TwoGenStatus { compatibility: i32, corners_two: bool, corners_pseudo: bool }

fn run_karn_bridge(input: &str, position: Option<&str>, generator: bool, convert_to_karn: bool) -> Result<String, String> {
    let input = std::ffi::CString::new(input).map_err(|_| "Input contains a NUL byte")?;
    let position_c = std::ffi::CString::new(position.unwrap_or("")).map_err(|_| "Position contains a NUL byte")?;
    let mut output = vec![0_i8; input.as_bytes().len().saturating_mul(16).max(4096)];
    let result = unsafe {
        match (convert_to_karn, position) {
            (false, _) => sq1_unkarnify(input.as_ptr(), output.as_mut_ptr(), output.len()),
            (true, Some(_)) => sq1_karnify_smart(input.as_ptr(), position_c.as_ptr(), generator, output.as_mut_ptr(), output.len()),
            (true, None) => sq1_karnify(input.as_ptr(), output.as_mut_ptr(), output.len()),
        }
    };
    if result < 0 { return Err("Karnotation conversion failed".into()); }
    if result as usize > output.len() {
        output.resize(result as usize, 0);
        let retry = unsafe {
            match (convert_to_karn, position) {
                (false, _) => sq1_unkarnify(input.as_ptr(), output.as_mut_ptr(), output.len()),
                (true, Some(_)) => sq1_karnify_smart(input.as_ptr(), position_c.as_ptr(), generator, output.as_mut_ptr(), output.len()),
                (true, None) => sq1_karnify(input.as_ptr(), output.as_mut_ptr(), output.len()),
            }
        };
        if retry < 0 { return Err("Karnotation conversion failed".into()); }
    }
    unsafe { std::ffi::CStr::from_ptr(output.as_ptr()) }.to_str().map(str::to_owned).map_err(|e| e.to_string())
}

#[tauri::command]
fn unkarnify(input: String) -> Result<String, String> {
    run_karn_bridge(&input, None, false, false)
}

#[tauri::command]
fn karnify(input: String, position: Option<String>, generator: bool) -> Result<String, String> {
    run_karn_bridge(&input, position.as_deref(), generator, true)
}

#[tauri::command]
fn rate_algorithm(algorithm: String, initial_top_a: bool) -> Result<RatingResult, String> {
    let algorithm = CString::new(algorithm).map_err(|_| "Algorithm contains a NUL byte")?;
    let mut result = RatingResult::default();
    let valid = unsafe { sq1_rate_algorithm(algorithm.as_ptr(), initial_top_a, &mut result) };
    result.valid = valid;
    Ok(result)
}

#[tauri::command]
fn two_gen_status(position: Vec<i32>) -> Result<TwoGenStatus, String> {
    if position.len() != 24 { return Err("A Square-1 position must have 24 slots".into()); }
    let mut corners_two = false;
    let mut corners_pseudo = false;
    let compatibility = unsafe { sq1_two_gen_compatibility(position.as_ptr(), &mut corners_two, &mut corners_pseudo) };
    Ok(TwoGenStatus { compatibility, corners_two, corners_pseudo })
}

#[derive(Clone, Default)]
struct SolverState(std::sync::Arc<std::sync::Mutex<()>>);

#[derive(Serialize)]
struct SolverResult {
    code: Option<i32>,
    stdout: String,
    stderr: String,
}

fn seed_pruning_tables(target: &PathBuf, source: &PathBuf) -> Result<(), String> {
    if target.join("sq1p1u.dat").exists() || !source.exists() {
        return Ok(());
    }
    fs::create_dir_all(target).map_err(|e| e.to_string())?;
    for entry in fs::read_dir(source).map_err(|e| e.to_string())? {
        let entry = entry.map_err(|e| e.to_string())?;
        let path = entry.path();
        if path.extension().is_some_and(|extension| extension == "dat") {
            fs::copy(&path, target.join(entry.file_name())).map_err(|e| e.to_string())?;
        }
    }
    Ok(())
}

extern "C" fn solver_line_callback(line: *const c_char, context: *mut c_void) {
    if line.is_null() || context.is_null() { return; }
    let channel = unsafe { &*(context as *const Channel<String>) };
    let value = unsafe { CStr::from_ptr(line) }.to_string_lossy().into_owned();
    let _ = channel.send(value);
}

fn solve_blocking(app: tauri::AppHandle, state: SolverState, position: String, flags: Vec<String>, on_line: Channel<String>) -> Result<SolverResult, String> {
    let _solver_guard = state.0.lock().map_err(|_| "Solver state is unavailable")?;
    let app_table_dir = app.path().app_data_dir().map_err(|e| e.to_string())?.join("pruning-tables");
    let bundled_tables = app.path().resource_dir().map_err(|e| e.to_string())?.join("resources/pruning-tables");
    let legacy_tables = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../legacy/build/Desktop-Debug/pruning-tables");
    let table_dir = if cfg!(debug_assertions) && legacy_tables.join("sq1p1u.dat").exists() {
        legacy_tables
    } else if bundled_tables.join("sq1p1u.dat").exists() {
        bundled_tables
    } else {
        seed_pruning_tables(&app_table_dir, &bundled_tables)?;
        app_table_dir
    };
    fs::create_dir_all(&table_dir).map_err(|e| e.to_string())?;
    let mut arguments = vec!["sq1opt".to_owned(), "-v5".to_owned()];
    arguments.extend(flags);
    arguments.push(position);
    let c_arguments = arguments.into_iter().map(|value| CString::new(value).map_err(|_| "Solver argument contains a NUL byte")).collect::<Result<Vec<_>, _>>()?;
    let pointers = c_arguments.iter().map(|value| value.as_ptr()).collect::<Vec<_>>();
    let table_directory = CString::new(table_dir.to_string_lossy().as_bytes()).map_err(|_| "Table path contains a NUL byte")?;
    let mut code = -1;
    let channel_context = &on_line as *const Channel<String> as *mut c_void;
    let output_pointer = unsafe { sq1_run_alloc(pointers.len() as i32, pointers.as_ptr(), table_directory.as_ptr(), &mut code, solver_line_callback, channel_context) };
    if output_pointer.is_null() { return Err("The embedded sq1opt solver returned no output".into()); }
    let output = unsafe { CStr::from_ptr(output_pointer) }.to_string_lossy().into_owned();
    unsafe { sq1_free_string(output_pointer); }
    Ok(SolverResult { code: Some(code), stdout: output, stderr: String::new() })
}

#[tauri::command]
async fn solve(app: tauri::AppHandle, state: State<'_, SolverState>, position: String, flags: Vec<String>, on_line: Channel<String>) -> Result<SolverResult, String> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || solve_blocking(app, state, position, flags, on_line))
        .await
        .map_err(|e| e.to_string())?
}

#[tauri::command]
fn stop_solver(state: State<'_, SolverState>) -> Result<(), String> {
    let _ = state;
    unsafe { sq1_request_stop(); }
    Ok(())
}

pub fn run() {
    tauri::Builder::default()
        .manage(SolverState::default())
        .invoke_handler(tauri::generate_handler![solve, stop_solver, unkarnify, karnify, rate_algorithm, two_gen_status])
        .run(tauri::generate_context!())
        .expect("error while running Solve-A-Squan");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn legacy_karn_bridge_round_trips_known_algorithm() {
        let numeric = run_karn_bridge("1,0 / 3,3 / 0,-3", None, false, false).unwrap();
        assert!(numeric.contains('/'));
        let karn = run_karn_bridge(&numeric, None, false, true).unwrap();
        assert!(!karn.trim().is_empty());
    }

    #[test]
    fn legacy_karn_bridge_expands_named_moves() {
        let numeric = run_karn_bridge("U D'", None, false, false).unwrap();
        assert!(numeric.chars().any(|character| character.is_ascii_digit()));
    }

    #[test]
    fn embedded_solver_bridge_runs_sq1opt_cpp() {
        let arguments = [CString::new("sq1opt").unwrap(), CString::new("-h").unwrap()];
        let pointers = arguments.iter().map(|argument| argument.as_ptr()).collect::<Vec<_>>();
        let directory = CString::new("").unwrap();
        let mut code = -1;
        extern "C" fn ignore_line(_: *const c_char, _: *mut c_void) {}
        let output = unsafe { sq1_run_alloc(2, pointers.as_ptr(), directory.as_ptr(), &mut code, ignore_line, std::ptr::null_mut()) };
        assert!(!output.is_null());
        let text = unsafe { CStr::from_ptr(output) }.to_string_lossy().into_owned();
        unsafe { sq1_free_string(output); }
        assert_eq!(code, 0);
        assert!(text.to_ascii_lowercase().contains("usage") || text.to_ascii_lowercase().contains("square"));
    }

    #[test]
    fn embedded_solver_uses_existing_tables_and_returns_a_solution() {
        let arguments = ["sq1opt", "-v5", "-es", "A1B2C3D45E6F7G8H-"]
            .map(|value| CString::new(value).unwrap());
        let pointers = arguments.iter().map(|argument| argument.as_ptr()).collect::<Vec<_>>();
        let tables = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../legacy/build/Desktop-Debug/pruning-tables");
        let directory = CString::new(tables.to_string_lossy().as_bytes()).unwrap();
        let mut code = -1;
        extern "C" fn ignore_line(_: *const c_char, _: *mut c_void) {}
        let output = unsafe { sq1_run_alloc(4, pointers.as_ptr(), directory.as_ptr(), &mut code, ignore_line, std::ptr::null_mut()) };
        assert!(!output.is_null());
        let text = unsafe { CStr::from_ptr(output) }.to_string_lossy().into_owned();
        unsafe { sq1_free_string(output); }
        assert_eq!(code, 0);
        assert!(text.contains('[') && text.contains(']'), "solver output did not contain a solution: {text}");
    }

    #[test]
    fn legacy_ergonomic_rating_bridge_returns_breakdown() {
        let rating = rate_algorithm("0,0/3,3/0,0".into(), false).unwrap();
        assert!(rating.valid);
        assert_eq!(rating.slice_count, 2);
        assert_ne!(rating.slice_start, 0);
    }

    #[test]
    fn shared_two_gen_constraint_check_accepts_solved_position() {
        let solved = vec![0, 0, 8, 1, 1, 9, 2, 2, 10, 3, 3, 11, 12, 4, 4, 13, 5, 5, 14, 6, 6, 15, 7, 7];
        let status = two_gen_status(solved).unwrap();
        assert_eq!(status.compatibility, 2);
        assert!(status.corners_two && status.corners_pseudo);
    }
}
