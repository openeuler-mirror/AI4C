if(WIN32)
  set(_check_files
    [[lib3]]
    [[lib3/(lib)?lib3\.(dll\.a|lib|l)]]
    [[lib4]]
    [[lib4/(lib)?lib4\.dll]]
    [[mybin]]
    [[mybin/exe\.exe]]
    [[mybin/(lib)?lib1\.dll]]
    [[myinclude]]
    [[myinclude/obj3\.h]]
    [[mylib]]
    [[mylib/(lib)?lib1\.(dll\.a|lib|l)]]
    [[mylib/(lib)?lib2\.(a|lib|l)]]
    )
elseif(MSYS)
  set(_check_files
    [[lib3]]
    [[lib3/liblib3\.dll\.a]]
    [[lib4]]
    [[lib4/msys-lib4\.dll]]
    [[mybin]]
    [[mybin/exe\.exe]]
    [[mybin/msys-lib1\.dll]]
    [[myinclude]]
    [[myinclude/obj3\.h]]
    [[mylib]]
    [[mylib/liblib1\.dll\.a]]
    [[mylib/liblib2\.a]]
    )
elseif(CYGWIN)
  set(_check_files
    [[lib3]]
    [[lib3/liblib3\.dll\.a]]
    [[lib4]]
    [[lib4/cyglib4\.dll]]
    [[mybin]]
    [[mybin/cyglib1\.dll]]
    [[mybin/exe\.exe]]
    [[myinclude]]
    [[myinclude/obj3\.h]]
    [[mylib]]
    [[mylib/liblib1\.dll\.a]]
    [[mylib/liblib2\.a]]
    )
else()
  set(_check_files
    [[lib3]]
    [[lib3/liblib3\.(dylib|so)]]
    [[lib4]]
    [[lib4/liblib4\.(dylib|so)]]
    [[mybin]]
    [[mybin/exe]]
    [[myinclude]]
    [[myinclude/obj3\.h]]
    [[mylib]]
    [[mylib/liblib1\.(dylib|so)]]
    [[mylib/liblib2\.a]]
    )
endif()
check_installed("^${_check_files}$")