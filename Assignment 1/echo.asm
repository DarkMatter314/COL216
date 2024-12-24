.text
.globl main
main:
li $v0, 9       #To obtain address of a memory location in heap, instruction is 9
syscall         #System call for address in heap
move $s0, $v0   #Store address of string location in $s0
li $v0, 8       #To input a string instruction is 8
move $a0, $s0   #Take the input string and store it in address $s0
li $a1, 101     #Allocate memory for string, if 101 characters not reached it will read till \n
syscall         #System call for input
li $v0, 4       #To output a string instruction is 4
move $a0, $s0   #Address of string on heap is $s0
syscall         #System call for output
li $v0, 10      #Instruction for exit code is 10
syscall         #Exit