-- xmake build for CommonLibF4RD, which ships CMake + vcpkg only. Upstream is a
-- pristine nested submodule and is not modified.
--
--     includes("lib/commonlibf4rd")
--     target(...) add_deps("commonlibf4rd")
--
-- An F4SE plugin laid out as src/**.cpp with a src/pch.h can take the whole build
-- from the rule instead:
--
--     includes("lib/commonlibf4rd")
--     set_project("MyPlugin")
--     add_rules("mode.debug", "mode.releasedbg", "mode.release")
--     target("MyPlugin")
--         add_rules("commonlibf4rd.plugin")
--
-- The mode rules define NDEBUG, which WIO::SetVerbose and WIO::Init are compiled under.

set_xmakever("3.0.0")

set_project("commonlibf4rd")
set_languages("c++23")  -- upstream requires cxx_std_20; our plugins are C++23
set_warnings("allextra")
set_encodings("utf-8")

add_rules("mode.debug", "mode.releasedbg", "mode.release")

-- Upstream's five dependencies, all from xmake-repo.
--
-- spdlog and fmt must be compiled, not header-only: header-only spdlog.h includes
-- <windows.h>, whose MEM_RELEASE / PAGE_EXECUTE_READWRITE macros collide with
-- F4SE/Impl/WinAPI.h. Boost is header-only (boost/stl_interfaces only).
add_requires("spdlog", { configs = { header_only = false, fmt_external = true } })
add_requires("fmt", { configs = { header_only = false } })
add_requires("zydis", "rsm-mmio")
add_requires("boost", { configs = { header_only = true } })

-- Absolute: a relative add_includedirs() resolves against the consuming project's
-- dir, not this script's.
local DIR = os.scriptdir()
local ROOT = path.join(DIR, "CommonLibF4RD/CommonLibF4")

target("commonlibf4rd", function()
    set_kind("static")
    set_default(os.scriptdir() == os.projectdir())

    add_packages("fmt", "spdlog", "zydis", "rsm-mmio", "boost", { public = true })

    add_files(path.join(ROOT, "src/**.cpp"))
    add_includedirs(path.join(ROOT, "include"), { public = true })

    -- Compatibility layer. Public so mods can include
    -- <WattzIO/Compat.h> from their pch.
    add_includedirs(path.join(DIR, "compat"), { public = true })
    add_headerfiles(path.join(DIR, "compat/(WattzIO/**.h)"))
    add_headerfiles(
        path.join(ROOT, "include/(F4SE/**.h)"),
        path.join(ROOT, "include/(RE/**.h)"),
        path.join(ROOT, "include/(REL/**.h)")
    )

    -- src/REL/*.h are private implementation headers included by their own .cpp
    add_includedirs(path.join(ROOT, "src"))

    -- PUBLIC in upstream's CMakeLists.txt.
    add_defines(
        "BOOST_STL_INTERFACES_DISABLE_CONCEPTS",
        "WINVER=0x0601",      -- Windows 7, the minimum Fallout 4 supports
        "_WIN32_WINNT=0x0601",
        { public = true }
    )

    -- Upstream names only Version.lib. F4SE::log::log_directory also needs
    -- SHGetKnownFolderPath + CoTaskMemFree, which MSVC's default CMake link line
    -- supplies and xmake does not.
    add_syslinks("Version", "Ole32", "Shell32", { public = true })

    -- PUBLIC so consuming projects inherit them.
    --   /utf-8          : fmt 12.x static_asserts without it.
    --   aligned_storage : RE/msvc/functional.h uses std::aligned_storage_t, which
    --                     C++23 deprecates; upstream builds at C++20.
    add_defines("_SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING", { public = true })

    if is_plat("windows") then
        add_cxxflags("/utf-8", { public = true })
        -- PUBLIC in upstream's CMakeLists.txt
        add_cxxflags("/permissive-", "/wd4200", "/wd4201", "/wd4324", { public = true })
        -- PRIVATE in upstream's CMakeLists.txt
        add_cxxflags(
            "/we4715",
            "/wd4061", "/wd4263", "/wd4264", "/wd4266", "/wd4371", "/wd4514",
            "/wd4582", "/wd4583", "/wd4623", "/wd4625", "/wd4626", "/wd4686",
            "/wd4820", "/wd5027", "/wd5045", "/wd5053", "/wd5204", "/wd5220"
        )
        -- Upstream headers trip this under allextra.
        add_cxxflags("/wd4100")
    end

    add_extrafiles(path.join(ROOT, "CommonLibF4.natvis"))
    set_pcxxheader(path.join(ROOT, "include/F4SE/Impl/PCH.h"))
end)

-- Build settings for an F4SE plugin target, derived from the target's name and script
-- directory. Settings specific to one plugin belong in that plugin's own xmake.lua.
rule("commonlibf4rd.plugin", function()
    on_load(function(target)
        local dir = target:scriptdir()

        -- on_load runs after the target's own script, so these fill in only what the
        -- target has not set. target:get() is nil for a value the target never set;
        -- target:kind() reports xmake's default instead.
        local function default(name, value)
            if not target:get(name) then
                target:set(name, value)
            end
        end

        default("kind", "shared")
        -- Project-scope settings in this file do not reach a consuming project through
        -- includes(), so these are set on the target. Upstream's PCH.h requires C++20
        -- or later.
        default("languages", "c++23")
        default("warnings", "allextra")
        default("encodings", "utf-8")

        target:add("deps", "commonlibf4rd")
        target:add("files", path.join(dir, "src/**.cpp"))
        target:add("headerfiles", path.join(dir, "src/**.h"))
        target:add("includedirs", path.join(dir, "src"))

        local pch = path.join(dir, "src/pch.h")
        if os.isfile(pch) then
            default("pcxxheader", pch)
        end
    end)
end)
