.text
.globl main
.globl exp

#Function to calculate exponent recursively in O(log n)
exp:
addi $sp, $sp, -4          #Create memory in the stack  
sw $ra, 0($sp)             #Store $ra for recursive function call
beq $a1, $zero, return     #If n is zero jump to return statement
andi $t1, $a1, 1           #Check if n is odd by 'and' with 1
bne $t1, $zero, odd        #If result of and is not zero, it is odd
srl $a1, $a1, 1            #If even, then calculate n/2
jal exp                    #Recursively call exp function to calculate x^(n/2)
mul $t2, $v0, $v0          #x^n = (x^(n/2))*(x^(n/2))
move $v0, $t2              #Store in v0 for returning the function call
lw $ra, 0($sp)             #Load the return address from the stack
addi $sp, $sp, 4           #Restore the state of the stack
jr $ra                     #Exit the function call
odd:
addi $a1, $a1, -1          #If n is odd calculate n-1
jal exp                    #Recursively call exp function to calculate x^(n-1)
mul $t2, $a0, $v0          #x^n = x*(x^(n-1))
move $v0, $t2              #Store in v0 for returning the function call
lw $ra, 0($sp)             #Load the return address from the stack
addi $sp, $sp, 4           #Restore the state of the stack
jr $ra                     #Exit the function call
return:
li $v0, 1                  #If n is 0, answer is 1
addi $sp, $sp, 4           #Restore the state of the stack
jr $ra                     #Exit the function call

main:
#Take inputs from user calculate the exponent and print the result
li $v0, 5     
syscall                 #Take input 'x' from user
move $a0, $v0           #Store x in $a0 for function call
li $v0, 5               
syscall                 #Take input 'n' from user
move $a1, $v0           #Store n in $a1 for function call
jal exp                 #Call the exponentiation function
move $a0, $v0           #Store the result in $v0 to $a0
li $v0, 1
syscall                 #Ouptut the answer
li $v0, 10
syscall                 #Exit
