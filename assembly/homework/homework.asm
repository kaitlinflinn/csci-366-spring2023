//==========================================================================================
// PROBLEM 1: Counting Up From 1 to 10 in Little Man Assembly
//==========================================================================================
      
        LDA ONE
        SUB TEN
LOOP    BRZ EXIT
        ADD TEN
        OUT
        ADD ONE
        SUB TEN
        BRA LOOP
EXIT    HLT
ONE     DAT 1
TEN     DAT 11

//==========================================================================================
// PROBLEM 2: Counting Up From 1 to 10 in MIPS Assembly
//==========================================================================================

// You can develop your code via the MARS emulator and then paste it in here
// Please use a jump in your solution
// Please use sys-calls to output the value
