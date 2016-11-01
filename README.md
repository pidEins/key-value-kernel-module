# Kernel-Module
Kernel Device driver to simulate hash operations.

1. To build the kernel module, go to kernel_module directory

   a. make 
   b. make install 

   Note: make install will copy few config files to a particular location. 

2. To build a share library which benchmark and validate binaries needs. 

   a. make 
   b. make install 

   Note: make install will copy the shared library to the /usr/local/lib location. 

3. To build th benchmark and validate binary

   a. make
   b. run the benchmark ex: ./benchmark 10 10
   c. run the validate ex: ./validate 10
      Now copy paste the output of the benchmark and then press ctl+D. 
      If values are correct then it will print "you passed". 
      else it will tell you the diff of actual value and expected value.  

