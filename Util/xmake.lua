add_rules("mode.debug", "mode.release")

target("Util")
    add_rules("qt.static")
    add_includedirs(".", {public = true})
    add_files("*.cpp")
    add_files("*.h")
    add_deps("Protocol")
    add_frameworks("QtCore")
