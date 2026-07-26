fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() > 1 {
        croissant_lib::run_cli(args);
    } else {
        croissant_lib::run();
    }
}
