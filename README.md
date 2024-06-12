# CDFGPass
LLVM Pass for generating a Control Data Flow Graph

Steps for Running the Pass:    
Open terminal    
mkdir build  
cd build  
cmake ..  
make  
cd ..  
opt -load "Path"/build/CDFG/libDFGPass.so -CDFGPass bubblesort.ll -enable-new-pm=0  

