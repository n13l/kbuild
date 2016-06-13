export PATH=$PWD/obj/lib/dummy:$PATH
export LD_LIBRARY_PATH=$PWD/obj/lib/dummy
export DYLD_LIBRARY_PATH==$PWD/obj/lib/dummy
java -Djava.library.path="./obj/lib/dummy" -cp "./obj/lib/dummy/libdummy-1.0.1.jar" com.dummy.Test
