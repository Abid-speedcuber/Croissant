fn main() {
    cc::Build::new()
        .cpp(true)
        .std("c++17")
        .file("native/karn_bridge.cpp")
        .include("../../legacy/sq1-core")
        .compile("sq1_karn_bridge");
    cc::Build::new()
        .cpp(true)
        .std("c++17")
        .opt_level(3)
        .define("SQ1OPT_LIBRARY", None)
        .file("native/solver_bridge.cpp")
        .file("../../sq1opt/sq1opt.cpp")
        .include("../../legacy/sq1-core")
        .compile("sq1_solver_bridge");
    cc::Build::new()
        .cpp(true)
        .std("c++17")
        .opt_level(3)
        .define("SQ1OPT_NO_QT", None)
        .file("native/rating_bridge.cpp")
        .file("../../legacy/sq1-core/sq1-logic.cpp")
        .include("../../legacy/sq1-core")
        .compile("sq1_rating_bridge");
    println!("cargo:rerun-if-changed=native/karn_bridge.cpp");
    println!("cargo:rerun-if-changed=native/solver_bridge.cpp");
    println!("cargo:rerun-if-changed=native/rating_bridge.cpp");
    println!("cargo:rerun-if-changed=../../legacy/sq1-core/sq1-logic.cpp");
    println!("cargo:rerun-if-changed=../../legacy/sq1-core/sq1-logic.h");
    println!("cargo:rerun-if-changed=../../sq1opt/sq1opt.cpp");
    println!("cargo:rerun-if-changed=../../legacy/sq1-core/karnotation.h");
    tauri_build::build();
}
