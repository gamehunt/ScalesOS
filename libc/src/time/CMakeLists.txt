set(SOURCES 
	asctime.c
	clock.c
	ctime.c
	difftime.c
	localtime.c
	mktime.c
	strftime.c
	time.c
)

target_sources(c PRIVATE ${SOURCES})
