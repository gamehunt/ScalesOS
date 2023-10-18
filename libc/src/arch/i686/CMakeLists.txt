set(SOURCES 
    memxxx.asm
)

target_compile_definitions(common INTERFACE 
	HAS_ARCH_MEMCPY
	HAS_ARCH_MEMSET
)

target_sources(k PRIVATE ${SOURCES})
target_sources(c PRIVATE ${SOURCES})
