# Project Name : ZeroRunEncoder.0re

# Description : 
- Conditional probability-based conversion + Exponential-golomb coding.
- The program converts file data to include as many zero-runs as possible and encodes them.

# Environment :
- Windows 10 x64
- Dev-C++ 5.11
- GCC 4.9.2 64-bit

# Usage : 
- Encoding Mode : ZeroRunEncoder.exe "encoding" InputFilePath OutputFilePath
- Decoding Mode : ZeroRunEncoder.exe "decoding" 0reFilePath OutputDirPath
- Show Details Mode : ZeroRunEncoder.exe "details" 0reFilePath

# Requirements : 
- To compile this program, the following requirements must be met.
- Linker Option (For Windows) : -Wl,--stack,67108864
- Compiler Option : -pthread
- Include File DirPath : ZeroRunEncoder.0re/include

# Auther : ESonia
