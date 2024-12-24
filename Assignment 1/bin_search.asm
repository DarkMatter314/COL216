.data
# Initialize the messages
yes: .asciiz "Yes at index "
no: .asciiz "Not Found"

.text
.globl main
#s0 - 'n', s2 - address of array[0], s3 - x, s4 - result
#t0 - low, t1 - mid, t2 - high, t3 - array[mid]
#s1, t4, t5 - Temporary Registers

main:
#Take input n from user
li $v0, 5                 #Take 'n' input from user
syscall
move $s0, $v0             #Store 'n' in $s0
sll $a0, $s0, 2           #Store number of bytes required by array
li $v0, 9   
syscall                   #Allocate appropriate memory on heap
move $s2, $v0             #Store address of array memory in $s2

#Take the array input using a loop
li $s1, 0                 #Initialize counter
Input:                    #Loop start for taking array input
beq $s1, $s0, Continue    #Check if counter = n
sll $t1, $s1, 2           
add $t1, $t1, $s2         #Find address to store integer in (array[i])
li $v0, 5                 #Input the integer
syscall                                     
sw $v0, 0($t1)            #Store the integer at the particular address
addi $s1, $s1, 1          #Increment the counter
j Input                   #Loop to the beginning
Continue:                   
li $v0, 5                 #Take input 'x' from the user
syscall
move $s3, $v0             #Store 'x' in $s3

#Start Binary Search
li $s4, -1                #Initialize a register to store the result with -2
move $t0, $zero           #Initalize $t0 as zero for 'low' 
addi $t2, $s0, -1         #Initialize $t2 as n-1 for 'high'
BinSearch:                #Start the loop for Binary Search
slt $t5, $t2, $t0         #Check if high<low 
bne $t5, $zero, End       #If high<low then jump to the end since binary search is over
add $t1, $t0, $t2         #Store mid in $t1
srl $t1, $t1, 1           #Dividing by 2 and taking only integer cmponent is same as shifting right by 1
sll $t4, $t1, 2           #Number of bytes ahead to look in the array
add $t4, $t4, $s2         #Store address of array[mid] in $t4
lw $t3, 0($t4)            #Load array[mid] in $t3
beq $t3, $s3, Equal       #If array[mid]=x then jump to 'Equal' branch
slt $t5, $t3, $s3         #Else check if arr[mid]<x
beq $t5, $zero, Greater   #Jump to greater if arr[mid]>x
addi $t0, $t1, 1          #low = mid+1 to check in the upper half since arr[mid]<x
j BinSearch               #Loop to the beginning
Greater:
addi $t2, $t1, -1         #high = mid-1 to check in the lower half of the array since arr[mid]>x
j BinSearch               #Loop to the beginning
Equal:                      
move $s4, $t1             #Store a potential match in $s4 since array[mid]=x
addi $t2, $t1, -1         #High=mid-1 since there can exist x before index mid
j BinSearch               #Loop to the beginning to check for first occurence

#Print the appropriate results
End:
slt $t5, $s4, $zero       #If result is -1 then it is less than 0
bne $t5, $zero, Not       #If result less than 0 then jump to Not Found statement
li $v0, 4                 #Else print the yes message and then the index
la $a0, yes
syscall                   #Print the yes message
li $v0, 1
move $a0, $s4
syscall                   #Print the index
j Exit                    #Jump to the exit condition of code
Not:        
li $v0, 4
la $a0, no      
syscall                   #Print the no message if $s4 is -1
Exit:
li $v0, 10                
syscall                   #Exit