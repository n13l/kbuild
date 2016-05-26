%module(package="com.dummy", 
        jniclassname="JNI",
        docstring="Bindings for DUMMY library") "Native"

%javaconst(1);

%include "ctor.i"
%include "typemaps.i"
%include "cpointer.i"

%rename(DUMMY) dummy;

%rename("%(strip:[ENDPOINT_])s") "";

%nodefaultctor dummy;
%ignore dummy_new;
%ignore dummy_free;

%{
struct dummy {} ;
#include "lib.h"
#include "jnu.h"
%}

struct dummy { 
        %extend {
                dummy() { 
                        struct dummy *dummy = dummy_new(); 
                        return (struct dummy*)dummy;
                }
                ~dummy() { 
                        dummy_free((struct dummy*)self);
                }
        }

};

%include "../lib.h"
