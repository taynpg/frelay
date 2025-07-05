add_rules("mode.debug", "mode.release")

target("crashelper")
    add_rules("qt.static")
    add_includedirs("include", {public = true})
    add_files("src/*.cxx")
    add_frameworks("QtCore")
