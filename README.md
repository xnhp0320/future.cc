# future.cc

future.cc: 是第一个版本，基于STL future/promise，好处是可以跨线程执行，因为stl本身的futrue/promise是可以的

future2.cc: 是第二个版本，不基于STL，不再提供thread-safe. 
增加了多参数支持。
增加了多参数时，即可以支持tuple也可以支持多参数
