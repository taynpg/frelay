add_rules("mode.debug", "mode.release")

target("Struct")
    add_rules("qt.static")
    add_includedirs(".", {public = true})
    add_files("*.h")
    add_files("*.cpp")
    add_frameworks("QtCore")
