add_library(CDFGPass MODULE
    CDFGPass.cpp
)

target_compile_features(CDFGPass PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(CDFGPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

if(APPLE)
    set_target_properties(CDFGPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif(APPLE)
