set_project("weqeqq.terminal")
set_version("0.1.0")
set_languages("c++23")
set_warnings("all")
add_rules("mode.debug", "mode.release", "plugin.compile_commands.autoupdate")

set_policy("build.c++.modules", true)
set_policy("build.c++.modules.std", false)

option("tests")
    set_default(false)
    set_showmenu(true)
    set_description("Enable building tests")
option_end()

add_repositories("weqeqq.repo https://github.com/weqeqq/xmake-repo.git")
if has_config("tests") then
    add_requires("weqeqq.test 0.3.5")
end

-- stylua: ignore start

target("weqeqq.terminal")
    set_kind("static")
    add_files("sources/**.cppm", {public = true})
    add_includedirs("sources", {public = true})
    set_policy("build.c++.modules", true)

if has_config("tests") then
target("tests")
    set_kind("binary")
    set_default(false)
    add_deps("weqeqq.terminal")
    add_files("tests/**.cpp")
    set_policy("build.c++.modules", true)
    add_packages("weqeqq.test", { components = {"core", "main"} })
    add_tests("default")
end
