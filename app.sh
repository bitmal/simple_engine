#!/usr/bin/sh

if [ "$1" == "--valgrind" ] 
then
    valgrind --verbose --timestamp=yes ./build/simple_engine 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >& \
        ./valgrind.log
    cat ./valgrind.log
else
    if [ "$1" == "--gdb" ] 
    then
        gdb ./build/simple_engine 2>&1 | ts '[%Y-%m-%d %H:%M:%S]'
    else
        if [ "$1" == "--build" ] 
        then
            echo "Selected User Option: 'build'" | ts '[%Y-%m-%d %H:%M:%S]'
            echo 
            rm -rf ./build
            mkdir ./build
            pushd ./build

            if [ "$2" == "--verbose" ]
            then
                gcc -std=c11 -g -Wall -pedantic -o simple_engine -L/home/derek/.src/simple_engine/lib/cimgui \
                    Wl,--enable-new-dtags,-rpath,/home/derek/.src/simple_engine/lib/cimgui ../src/engine/main.c \
                    lm -lGL -lSDL3 -lGLEW -lcimgui -ldl 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >& build.log
            else
                if [ "$2" == "--warning" ]
                then
                    gcc -std=c11 -g -pedantic -o simple_engine -L/home/derek/.src/simple_engine/lib/cimgui \
                        Wl,--enable-new-dtags,-rpath,/home/derek/.src/simple_engine/lib/cimgui ../src/engine/main.c \
                        lm -lGL -lSDL3 -lGLEW -lcimgui -ldl 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >& build.log
                else
                    gcc -std=c11 -g -o simple_engine -L/home/derek/.src/simple_engine/lib/cimgui \
                        Wl,--enable-new-dtags,-rpath,/home/derek/.src/simple_engine/lib/cimgui ../src/engine/main.c \
                        lm -lGL -lSDL3 -lGLEW -lcimgui -ldl 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >& ./build.log
                fi
            fi

            cat build.log

            popd
        else
            if [ "$1" == "--run" ]
            then
                echo "Selected User Option: 'run'" | ts '[%Y-%m-%d %H:%M:%S]'
                echo "Now running the engine!" | ts '[%Y-%m-%d %H:%M:%S]'
                ./build/simple_engine 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >& ./run_debug.log
            else
                echo "Warning: User failed to provide a runtime option, but out of sheer courtesy, the engine shall run in 'release' mode!"
                ./build/simple_engine 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >& ./run_debug.log
            fi

            cat ./run_debug.log
        fi
    fi
fi
