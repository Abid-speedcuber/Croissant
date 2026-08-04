fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() > 1 {
        if args[1] == "--version" || args[1] == "-v" {
            println!("croissant {}", env!("CARGO_PKG_VERSION"));
            return;
        }
        croissant_lib::run_cli(args);
    } else {
        croissant_lib::run();
    }
}
