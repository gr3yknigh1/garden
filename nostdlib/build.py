from hbuild import *

#
# Macros options:
#
#     - NOC_LIBC_WRAPPER - A bunch of libc's wrappers, which might be usefull.
#

noc = add_library("noc", sources=[
    "noc/src/abs.c",
    "noc/src/buf.c",
    "noc/src/countdigits.c",
    "noc/src/flt_charcount.c",
    "noc/src/from_str.c",
    "noc/src/http.c",
    "noc/src/io.c",
    "noc/src/memory.c",
    "noc/src/platform.c",
    "noc/src/str.c",
    "noc/src/to_str.c",
])
target_includes(noc, Access.PUBLIC, includes=["noc/include"])
target_macros(noc, macros=dict(
    NOC_DO_EXPORT="1",

    # NOTE(gr3yknigh1): While there is no option export in HBUILD, we define all options by default to
    # true [2025/06/10]
    NOC_LIBC_WRAPPERS="1",
))

testbed = add_executable("testbed", sources=("testbed.c",))
target_links(testbed, links=[noc])

add_package("noc", targets=[
    noc
])
