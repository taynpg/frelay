add_rules("mode.debug", "mode.release")

target("Protocol")
    add_rules("qt.static")
    add_includedirs(".", {public = true})
    add_files("*.cxx")
    add_files("*.h")
    add_deps("Struct")
    add_frameworks("QtCore")
