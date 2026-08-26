use serde::Serialize;
use std::{
    fs,
    ffi::{c_char, c_void, CStr, CString},
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
    fn sq1_two_gen_compatibility(position: *const i32, specific_angle_bot: bool, corners_two: *mut bool, corners_pseudo: *mut bool) -> i32;
    fn sq1_set_rating_weights(w1: f64, w2: f64, w3: f64, w4: f64);
    fn sq1_set_move_value(key: *const c_char, value: i32) -> bool;
    fn sq1_reset_rating_config();
    fn sq1_batch_init_alloc(argc: i32, argv: *const *const c_char, table_directory: *const c_char, exit_code: *mut i32,
                            callback: extern "C" fn(*const c_char, *mut c_void), callback_context: *mut c_void) -> *mut c_char;
    fn sq1_batch_solve_alloc(position: *const c_char, exit_code: *mut i32,
                             callback: extern "C" fn(*const c_char, *mut c_void), callback_context: *mut c_void) -> *mut c_char;
    fn sq1_batch_solve_multi_alloc(num_candidates: i32, candidates: *const *const c_char,
                                   exit_code: *mut i32,
                                   callback: extern "C" fn(*const c_char, *mut c_void), callback_context: *mut c_void) -> *mut c_char;
    fn sq1_batch_destroy_alloc();
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

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
struct TableInfo { name: String, size: u64 }

fn table_dir(app: &tauri::AppHandle) -> Result<std::path::PathBuf, String> {
    Ok(app.path().app_data_dir().map_err(|e| e.to_string())?.join("pruning-tables"))
}

fn is_table_file(name: &str) -> bool {
    !name.is_empty()
        && name.ends_with(".dat")
        && std::path::Path::new(name).file_name().map(|f| f == name).unwrap_or(false)
}

#[tauri::command]
fn list_pruning_tables(app: tauri::AppHandle) -> Vec<TableInfo> {
    let mut out = Vec::new();
    let dir = match table_dir(&app) {
        Ok(d) => d,
        Err(_) => return out,
    };
    if let Ok(rd) = fs::read_dir(dir) {
        for entry in rd.flatten() {
            let name = entry.file_name().to_string_lossy().into_owned();
            if is_table_file(&name) {
                if let Ok(meta) = entry.metadata() {
                    out.push(TableInfo { name, size: meta.len() });
                }
            }
        }
    }
    out.sort_by(|a, b| a.name.cmp(&b.name));
    out
}

#[tauri::command]
fn delete_pruning_table(app: tauri::AppHandle, name: String) -> Result<(), String> {
    if !is_table_file(&name) { return Err("Invalid table file name".into()); }
    let path = table_dir(&app)?.join(&name);
    fs::remove_file(path).map_err(|e| e.to_string())
}

#[tauri::command]
fn clear_pruning_tables(app: tauri::AppHandle) -> Result<(), String> {
    let dir = table_dir(&app)?;
    if let Ok(rd) = fs::read_dir(dir) {
        for entry in rd.flatten() {
            let name = entry.file_name().to_string_lossy().into_owned();
            if is_table_file(&name) { let _ = fs::remove_file(entry.path()); }
        }
    }
    Ok(())
}

#[tauri::command]
fn app_size(app: tauri::AppHandle) -> u64 {
    let exe = match std::env::current_exe() { Ok(e) => e, Err(_) => return 0 };
    let root = match exe.parent() { Some(d) => d.to_path_buf(), None => return 0 };
    let mut total = 0u64;
    let mut stack = vec![root];
    while let Some(dir) = stack.pop() {
        let Ok(rd) = fs::read_dir(dir) else { continue };
        for entry in rd.flatten() {
            let path = entry.path();
            if path.is_dir() {
                let skip = path.file_name().map(|n| n == "pruning-tables" || n == "node_modules").unwrap_or(false);
                if !skip { stack.push(path); }
            } else if let Ok(meta) = entry.metadata() {
                total += meta.len();
            }
        }
    }
    let _ = app;
    total
}

fn run_karn_bridge(input: &str, position: Option<&str>, generator: bool, convert_to_karn: bool) -> Result<String, String> {
    let input = std::ffi::CString::new(input).map_err(|_| "Input contains a NUL byte")?;
    let position_c = std::ffi::CString::new(position.unwrap_or("")).map_err(|_| "Position contains a NUL byte")?;
    let mut output = vec![0 as c_char; input.as_bytes().len().saturating_mul(16).max(4096)];
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

// Resets the ergonomics rater to defaults, then re-applies the given weights and move value overrides
#[tauri::command]
fn set_rating_config(weights: [f64; 4], move_values: std::collections::HashMap<String, i32>) -> Result<(), String> {
    unsafe { sq1_reset_rating_config(); }
    unsafe { sq1_set_rating_weights(weights[0], weights[1], weights[2], weights[3]); }
    for (key, value) in move_values {
        let key = CString::new(key).map_err(|_| "Move-value key contains a NUL byte")?;
        unsafe { sq1_set_move_value(key.as_ptr(), value); }
    }
    Ok(())
}

#[tauri::command]
fn two_gen_status(position: Vec<i32>, specific_angle_bot: bool) -> Result<TwoGenStatus, String> {
    if position.len() != 24 { return Err("A Square-1 position must have 24 slots".into()); }
    let mut corners_two = false;
    let mut corners_pseudo = false;
    let compatibility = unsafe { sq1_two_gen_compatibility(position.as_ptr(), specific_angle_bot, &mut corners_two, &mut corners_pseudo) };
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

extern "C" fn solver_line_callback(line: *const c_char, context: *mut c_void) {
    if line.is_null() || context.is_null() { return; }
    let channel = unsafe { &*(context as *const Channel<String>) };
    let value = unsafe { CStr::from_ptr(line) }.to_string_lossy().into_owned();
    let _ = channel.send(value);
}

fn solve_blocking(app: tauri::AppHandle, state: SolverState, position: String, flags: Vec<String>, on_line: Channel<String>) -> Result<SolverResult, String> {
    let _solver_guard = state.0.lock().map_err(|_| "Solver state is unavailable")?;
    let table_dir = app.path().app_data_dir().map_err(|e| e.to_string())?.join("pruning-tables");
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
    if output_pointer.is_null() { return Err("The solver returned no output".into()); }
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

fn batch_init_blocking(app: tauri::AppHandle, flags: Vec<String>) -> Result<SolverResult, String> {
    let table_dir = app.path().app_data_dir().map_err(|e| e.to_string())?.join("pruning-tables");
    fs::create_dir_all(&table_dir).map_err(|e| e.to_string())?;
    let mut arguments = vec!["sq1opt".to_owned(), "-v1".to_owned()];
    arguments.extend(flags);
    let c_arguments = arguments.into_iter().map(|value| CString::new(value).map_err(|_| "Batch init argument contains a NUL byte")).collect::<Result<Vec<_>, _>>()?;
    let pointers = c_arguments.iter().map(|value| value.as_ptr()).collect::<Vec<_>>();
    let table_directory = CString::new(table_dir.to_string_lossy().as_bytes()).map_err(|_| "Table path contains a NUL byte")?;
    let mut code = -1;
    extern "C" fn ignore_line(_: *const c_char, _: *mut c_void) {}
    let output_pointer = unsafe { sq1_batch_init_alloc(pointers.len() as i32, pointers.as_ptr(), table_directory.as_ptr(), &mut code, ignore_line, std::ptr::null_mut()) };
    if !output_pointer.is_null() {
        let output = unsafe { CStr::from_ptr(output_pointer) }.to_string_lossy().into_owned();
        unsafe { sq1_free_string(output_pointer); }
        Ok(SolverResult { code: Some(code), stdout: output, stderr: String::new() })
    } else {
        Ok(SolverResult { code: Some(code), stdout: String::new(), stderr: String::new() })
    }
}

#[tauri::command]
async fn batch_init(app: tauri::AppHandle, state: State<'_, SolverState>, flags: Vec<String>) -> Result<SolverResult, String> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let _solver_guard = state.0.lock().map_err(|_| "Solver state is unavailable")?;
        batch_init_blocking(app, flags)
    })
    .await
    .map_err(|e| e.to_string())?
}

fn batch_solve_blocking(position: String, on_line: Channel<String>) -> Result<SolverResult, String> {
    let position_c = CString::new(position).map_err(|_| "Position contains a NUL byte")?;
    let mut code = -1;
    let channel_context = &on_line as *const Channel<String> as *mut c_void;
    let output_pointer = unsafe { sq1_batch_solve_alloc(position_c.as_ptr(), &mut code, solver_line_callback, channel_context) };
    if output_pointer.is_null() { return Err("The batch solver returned no output".into()); }
    let output = unsafe { CStr::from_ptr(output_pointer) }.to_string_lossy().into_owned();
    unsafe { sq1_free_string(output_pointer); }
    Ok(SolverResult { code: Some(code), stdout: output, stderr: String::new() })
}

#[tauri::command]
async fn batch_solve_position(state: State<'_, SolverState>, position: String, on_line: Channel<String>) -> Result<SolverResult, String> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let _solver_guard = state.0.lock().map_err(|_| "Solver state is unavailable")?;
        batch_solve_blocking(position, on_line)
    })
    .await
    .map_err(|e| e.to_string())?
}

fn batch_solve_multi_blocking(candidates: Vec<String>, on_line: Channel<String>) -> Result<SolverResult, String> {
    let c_candidates: Vec<CString> = candidates.into_iter()
        .map(|s| CString::new(s).map_err(|_| "Candidate contains a NUL byte"))
        .collect::<Result<Vec<_>, _>>()?;
    let pointers: Vec<*const c_char> = c_candidates.iter().map(|c| c.as_ptr()).collect();
    let mut code = -1;
    let channel_context = &on_line as *const Channel<String> as *mut c_void;
    let output_pointer = unsafe {
        sq1_batch_solve_multi_alloc(pointers.len() as i32, pointers.as_ptr(), &mut code, solver_line_callback, channel_context)
    };
    if output_pointer.is_null() { return Err("The batch solver returned no output".into()); }
    let output = unsafe { CStr::from_ptr(output_pointer) }.to_string_lossy().into_owned();
    unsafe { sq1_free_string(output_pointer); }
    Ok(SolverResult { code: Some(code), stdout: output, stderr: String::new() })
}

#[tauri::command]
async fn batch_solve_multi(state: State<'_, SolverState>, candidates: Vec<String>, on_line: Channel<String>) -> Result<SolverResult, String> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let _solver_guard = state.0.lock().map_err(|_| "Solver state is unavailable")?;
        batch_solve_multi_blocking(candidates, on_line)
    })
    .await
    .map_err(|e| e.to_string())?
}

#[tauri::command]
fn batch_destroy(state: State<'_, SolverState>) -> Result<(), String> {
    let _solver_guard = state.0.lock().map_err(|_| "Solver state is unavailable")?;
    unsafe { sq1_batch_destroy_alloc(); }
    Ok(())
}

#[tauri::command]
fn stop_solver(state: State<'_, SolverState>) -> Result<(), String> {
    let _ = state;
    unsafe { sq1_request_stop(); }
    Ok(())
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_store::Builder::default().build())
        .manage(SolverState::default())
        .invoke_handler(tauri::generate_handler![solve, stop_solver, unkarnify, karnify, rate_algorithm, set_rating_config, two_gen_status, list_pruning_tables, delete_pruning_table, clear_pruning_tables, app_size, batch_init, batch_solve_position, batch_solve_multi, batch_destroy])
        .run(tauri::generate_context!())
        .expect("error while running Croissant");
}

pub fn run_cli(args: Vec<String>) -> ! {
    let mut solver_args = vec!["sq1opt".to_owned()];
    solver_args.extend(args.into_iter().skip(1));

    let c_args: Vec<CString> = solver_args
        .into_iter()
        .map(|a| CString::new(a).expect("Solver argument contains NUL byte"))
        .collect();
    let pointers: Vec<*const c_char> = c_args.iter().map(|a| a.as_ptr()).collect();

    let table_path = cli_table_dir();
    fs::create_dir_all(&table_path).expect("Failed to create pruning table directory");
    let table_c =
        CString::new(table_path.to_string_lossy().as_bytes()).expect("Table path contains NUL byte");

    extern "C" fn print_line(line: *const c_char, _ctx: *mut c_void) {
        if !line.is_null() {
            let s = unsafe { CStr::from_ptr(line) }.to_string_lossy();
            println!("{s}");
        }
    }

    let mut code: i32 = -1;
    let output = unsafe {
        sq1_run_alloc(
            pointers.len() as i32,
            pointers.as_ptr(),
            table_c.as_ptr(),
            &mut code,
            print_line,
            std::ptr::null_mut(),
        )
    };

    if !output.is_null() {
        unsafe {
            sq1_free_string(output);
        }
    }

    std::process::exit(code.max(0));
}

fn cli_table_dir() -> std::path::PathBuf {
    if let Ok(exe) = std::env::current_exe() {
        if let Some(dir) = exe.parent() {
            let bundled = dir.join("resources/pruning-tables");
            if bundled.join("sq1stt.dat").exists() {
                return bundled;
            }
            if let Some(grandparent) = dir.parent() {
                let alt = grandparent.join("resources/pruning-tables");
                if alt.join("sq1stt.dat").exists() {
                    return alt;
                }
            }
        }
    }
    let base = if let Ok(home) = std::env::var("HOME") {
        std::path::PathBuf::from(home).join(".local/share")
    } else {
        std::path::PathBuf::from(".")
    };
    base.join("com.croissant.desktop/pruning-tables")
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
        let tables = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("resources/pruning-tables");
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
    fn ergonomic_rating_accepts_compact_karn_solution_lines() {
        let rating = rate_algorithm("1-3/u d' m F W F' D' -54 56".into(), false).unwrap();
        assert!(rating.valid);
        assert_ne!(rating.slice_start, 0);
    }

    #[test]
    fn shared_two_gen_constraint_check_accepts_solved_position() {
        let solved = vec![0, 0, 8, 1, 1, 9, 2, 2, 10, 3, 3, 11, 12, 4, 4, 13, 5, 5, 14, 6, 6, 15, 7, 7];
        let status = two_gen_status(solved, false).unwrap();
        assert_eq!(status.compatibility, 2);
        assert!(status.corners_two && status.corners_pseudo);
    }
}
