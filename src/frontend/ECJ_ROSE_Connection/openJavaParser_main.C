#include "sage3basic.h"

#include <string.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>

#include "rose_config.h"

/* This is defined if ROSE is configured to use the Java Open Fortran Parser */
#ifndef USE_ROSE_JAVA_SUPPORT
#error "openJavaParser_main.C should not be compiled when Java support is disabled"
#endif

#include "jni.h"

// DQ (9/6/2010): Allow this to be commented out to simplify debugging using gdb.
// #define ENABLE_FORTRAN_ERROR_HANDLER

#ifdef ENABLE_FORTRAN_ERROR_HANDLER
#include "fortran_error_handler.h"
#endif

using namespace std;

#define ERROR_CODE 4

int runECJ (int argc, char **argv);

/* Include the paths to the source tree and build tree for ROSE */
#include "rose_config.h"

#include "rose_paths.h"
#include "commandline_processing.h"
#include "assert.h"

/* This is critical to permitting the correct library (the one just built by the user)
// to be used instead of requiring the LD_LIBRARY_PATH to beexplicitly set by the user
// before running any tests.  It is alos important to proper testing using "make distcheck"
// since we want the new library built using that rule to be tested over any other.
*/
// DQ (10/12/2010): commented out to debug using the environments LD_LIBRARY_PATH value.
// #define OVERWRITE_LD_LIBRARY_PATH 1
#define OVERWRITE_LD_LIBRARY_PATH 1

// DQ (8/15/2011): Moved to openJavaParser_main.C to
// separate the work on Java from the rest of ROSE and support the ROSE
// configuration language only options.
// DQ (10/21/2010): If Fortran is being supported then there will
// be a definition of this pointer there.  Note that we currently
// use only one pointer so that we can simplify how the JVM support 
// is used for either Fortran or Java language support.
#ifndef ROSE_BUILD_FORTRAN_LANGUAGE_SUPPORT
SgSourceFile *OpenFortranParser_globalFilePointer = NULL;
#endif

// DQ (8/15/2011): These functions were moved to openJavaParser_main.C to
// separate the work on Java from the rest of ROSE and support the ROSE
// configuration language only options.
list<SgScopeStatement *> astJavaScopeStack;

bool emptyJavaStateStack() {
    // Use the scope stack to indicate if we have a value scope available as part of Java lanaguage processing.
    return astJavaScopeStack.empty();
}


SgScopeStatement *getTopOfJavaScopeStack() {
    ROSE_ASSERT(astJavaScopeStack.empty() == false);
    SgScopeStatement* topOfStack = astJavaScopeStack.front();

    return topOfStack;
}



/* DQ (8/16/2007): This is what we want ROSE to call so that we can run the Java based OpenJavaParser from C++. */
int openJavaParser_main(int argc, char **argv) {
    /* Call the function that will start a JVM and call the OFP.  */

    /* To use different versions of the LD_LIBRARY_PATH, get the value, 
       change it to the path in the build tree, and then reset it to the 
       old value after the call to JVM.  We need to do this because the
       libparser_java_FortranParserActionJNI.so that we need is in the 
       build tree and it is specific to the configuration of ROSE (else 
       V_<class name> enum values will be different as a result of 
       configuration options that might trigger different numbers of IR 
       nodes to be use.
    */

    /* Overwite to a new value. It is not clear when to use the install path and when to use the build path! */
    // string new_value = findRoseSupportPathFromBuild("src/frontend/OpenFortranParser_SAGE_Connection/.libs", "lib");
    string new_value = findRoseSupportPathFromBuild("src/frontend/ECJ_ROSE_Connection/.libs", "lib");

    // This does not help or hurt.
    // new_value += ":" + findRoseSupportPathFromBuild("src/3rdPartyLibraries/java-parser", "lib");

    /* Save the old value */
    const char* old_value = getenv(ROSE_SHLIBPATH_VAR); // Might be null
    // assert(old_value != NULL);

#if OVERWRITE_LD_LIBRARY_PATH
    int overwrite = 1;
    int env_status = setenv(ROSE_SHLIBPATH_VAR,new_value.c_str(),overwrite);
    assert(env_status == 0);
#endif

    if (SgProject::get_verbose() > 0) {
        printf ("Call the function that will start a JVM and call the ECJ \n\n");

        // DQ (10/12/2010): Added debugging support...
        string JVM_command_line = CommandlineProcessing::generateStringFromArgList(CommandlineProcessing::generateArgListFromArgcArgv(argc, argv));
        printf ("Java JVM commandline = %s \n",JVM_command_line.c_str());
        printf ("ROSE modified %s = %s \n",ROSE_SHLIBPATH_VAR,new_value.c_str());
    }

#ifdef ENABLE_FORTRAN_ERROR_HANDLER
    fortran_error_handler_begin();
#endif

    int status = runECJ(argc, argv);

#ifdef ENABLE_FORTRAN_ERROR_HANDLER
    fortran_error_handler_end();
#endif

    if (SgProject::get_verbose() > 0) {
        printf ("JVM processing done.\n\n");
    }

    /* Reset to the saved value */
#if OVERWRITE_LD_LIBRARY_PATH
    // DQ (9/9/2011): Note that old_value can be NULL and if so then we don't want it to be dereferenced.
    // env_status = setenv(ROSE_SHLIBPATH_VAR,old_value,overwrite);
    if (old_value != NULL)
        env_status = setenv(ROSE_SHLIBPATH_VAR,old_value,overwrite);

    assert(env_status == 0);
#endif

    return status;
}

#include "jserver.h"
#include "ecj.h"

int runECJ (int argc, char **argv) {
    int retval= 0;
    /* start/find the Java VM with ofp method loaded,run ofp on the args */
    retval = jvm_ecj_processing(argc, argv);
  
    return retval;
}


