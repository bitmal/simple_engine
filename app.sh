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
                gcc -std=c11 -g -Wall -pedantic -o simple_engine -L/home/derek/.src/simple_engine/lib/cimgui \
                Wl,--enable-new-dtags,-rpath,/home/derek/.src/simple_engine/lib/cimgui ../src/engine/main.c \
                lm -lGL -lSDL3 -lGLEW -lcimgui -ldl >& build.log
            else
                if [ "$2" == "--warning" ]
                then
                    gcc -std=c11 -g -pedantic -o simple_engine -L/home/derek/.src/simple_engine/lib/cimgui \
                    Wl,--enable-new-dtags,-rpath,/home/derek/.src/simple_engine/lib/cimgui ../src/engine/main.c \
                    lm -lGL -lSDL3 -lGLEW -lcimgui -ldl >& build.log
                else
                    gcc -std=c11 -g -o simple_engine -L/home/derek/.src/simple_engine/lib/cimgui \
                    Wl,--enable-new-dtags,-rpath,/home/derek/.src/simple_engine/lib/cimgui ../src/engine/main.c \
                    lm -lGL -lSDL3 -lGLEW -lcimgui -ldl >& build.log
                fi
            fi

            cat build.log 2> exec echo

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
