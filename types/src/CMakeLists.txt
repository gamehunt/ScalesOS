set(SHARED_SOURCES 
	list.c
	tree.c
)

target_sources(ktypes PRIVATE ${SHARED_SOURCES})
target_sources(types PRIVATE ${SHARED_SOURCES})
target_sources(stypes PRIVATE ${SHARED_SOURCES})
