cmake_minimum_required(VERSION 3.23)

project(Scales-Shared-Headers)

install(DIRECTORY ./ 
        DESTINATION include/ 
        PATTERN "*.txt" EXCLUDE)