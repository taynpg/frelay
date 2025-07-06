add_rules("mode.debug", "mode.release")

set_version("0.2.1", {build = "1"})

if is_plat("windows") then
    add_cxflags("/utf-8")
end

if is_plat("mingw") then
    add_defines("NO_CRASHELPER=ON")
end

if has_config("xp") then
    add_defines("_WIN32_WINNT=0x0501")
else
    add_defines("_WIN32_WINNT=0x0601")
end

add_defines("FMT_HEADER_ONLY")
add_includedirs("3rd")

if has_config("gui") then
    add_includedirs("Gui/Control")
    includes("Gui", "3rd/SingleApplication-3.5.2")
end

add_includedirs("$(builddir)")
add_configfiles("version.h.in", {filename = "fversion.h", outputdir = "$(builddir)"})
includes("Struct", "Protocol", "Util", "ClientCore", "Console", "Server", "Test")

if is_plat("mingw") then
else
    includes("crashelper")
end

option("xp")
	set_default(false)
	set_showmenu(true)
    set_description("Enable Windows XP support")

option("gui")
    set_default(false)
    set_showmenu(true)
    set_description("Enable GUI support")

option("qt5")
    set_default(false)
    set_showmenu(true)
    set_description("Use Qt5 instead of Qt6.")