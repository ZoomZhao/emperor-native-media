// Native UI entry; embed only installed Apple CPython for own adapter RPC code.
#import <AppKit/AppKit.h>
#include <Python.h>
#include <stdio.h>
#include <unistd.h>
int main(int argc,char**argv){
 if(argc!=2||!NSThread.isMainThread)return 2;
 alarm(45);
 @autoreleasepool{
  [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
  fprintf(stderr,"S67_NATIVE_PREFIX main_thread=%d screens=%lu main_screen=%d\n",NSThread.isMainThread,(unsigned long)NSScreen.screens.count,NSScreen.mainScreen!=nil);fflush(stderr);
  Py_NoSiteFlag=1;Py_IsolatedFlag=1;Py_IgnoreEnvironmentFlag=1;Py_DontWriteBytecodeFlag=1;
  Py_SetPythonHome(L"<LOCAL_MACHINE_PATH>");
  Py_Initialize();FILE*f=fopen(argv[1],"rb");if(!f)return 3;
  int result=PyRun_SimpleFileEx(f,argv[1],1);int closed=Py_FinalizeEx();return result?4:closed?5:0;
 }
}
