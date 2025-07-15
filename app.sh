#!/usr/bin/sh

if [ "$1" == "--valgrind" ] 
then
    exec valgrind --verbose --timestamp=yes ./build/simple_engine 2> ./valgrind.log
else
    if [ "$1" == "--gdb" ] 
    then
        exec gdb ./build/simple_engine
    else
        if [ "$1" == "--build" ] 
        then
            echo "Selected User Option: 'build'"
            echo 
            rm -rf ./build
            mkdir ./build
            pushd ./build

            if [ "$2" == "--verbose" ]
            then
                exec gcc -std=c11 -Wall -g -o simple_engine ../src/engine/main.c -lm -lGL -lSDL3 -lGLEW -l"../../lib/cimgui/cimgui"
            else
                exec gcc -std=c11 -g -pedantic -o simple_engine -L/home/derek/.src/simple_engine/lib/cimgui -Wl,--enable-new-dtags,-rpath,/home/derek/.src/simple_engine/lib/cimgui ../src/engine/main.c -lm -lGL -lSDL3 -lGLEW -lcimgui #-ldl
            fi

            popd
        else
            if [ "$1" == "--run" ]
            then
                echo "Selected User Option: 'run'"
                echo "Now running the engine!"
                exec ./build/simple_engine
            else
                echo "Warning: User failed to provide a runtime option, but out of sheer courtesy, the engine shall run in 'release' mode!"
                exec ./build/simple_engine
            fi
        fi
    fi
fi
