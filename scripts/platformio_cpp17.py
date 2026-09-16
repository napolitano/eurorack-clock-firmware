"""Force the embedded PlatformIO project and libraries into GCC-7 C++17 mode.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0

PlatformIO's STM32Cube environment currently resolves to GCC 7.2.1.  That
compiler implements C++17 using the draft-era ``gnu++1z`` spelling.  This
POST script runs after the platform/framework environment has been built so a
later default language-standard flag cannot silently downgrade CLOCK back to
C++14.
"""

Import("env", "projenv")

CPP17_FLAG = "-std=gnu++1z"
_CPP_STANDARD_FLAGS = {
    "-std=gnu++98",
    "-std=c++98",
    "-std=gnu++11",
    "-std=c++11",
    "-std=gnu++14",
    "-std=c++14",
    "-std=gnu++17",
    "-std=c++17",
    "-std=gnu++1z",
    "-std=c++1z",
}


def _enforce_cpp17(build_env):
    """Remove competing C++ dialect flags and append the GCC-7 C++17 flag."""
    for key in ("CCFLAGS", "CXXFLAGS"):
        flags = list(build_env.get(key, []))
        filtered = [flag for flag in flags if str(flag) not in _CPP_STANDARD_FLAGS]
        build_env.Replace(**{key: filtered})
    build_env.Append(CXXFLAGS=[CPP17_FLAG])


# Global/framework environment and project sources.
_enforce_cpp17(env)
_enforce_cpp17(projenv)

# Libraries are isolated PlatformIO construction environments.  clock_core is
# one of them and must use the same language dialect as src/.
for library_builder in env.GetLibBuilders():
    _enforce_cpp17(library_builder.env)
