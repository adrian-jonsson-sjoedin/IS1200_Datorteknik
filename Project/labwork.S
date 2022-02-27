  # labwork.S
  # Written 2015-2017 by F Lundevall
  # Skeleton file for IS1200/IS1500 lab 1.
  # The macros PUSH and POP are in the public domain.
  # Please add your own code at the end of the file.

  #
  # Please keep the two macros PUSH and POP unchanged
  #

.global hexasc #added 1/2-22 adriansj and bafoday

.global delay #added 1/2-22 adriansj and bafoday

.global time2string #added 1/2-22 adriansj and bafoday


.macro	PUSH reg
	addi	$sp,$sp,-4
	sw	\reg,0($sp)
.endm

.macro	POP reg
	lw	\reg,0($sp)
	addi	$sp,$sp,4
.endm
  #
  .data
  .text
  # Please add your own code below this line 

  #added 1/2-22 adriansj and bafoday
  hexasc:
 	 li	$t0,0x09	#sets breakpoint
 	 andi 	$v0,$a0,0x0f	#masks everything except lsb in $a0 and save in $t0
 	 
 	 ble	$v0,$t0, numb	#if input value is less than 9 branch to numb
 	 nop
 	 
 	 addi	$v0,$v0,0x037	#add so that $v0 represents an ASCII number corresponding to a letter
 	 jr	$ra			#jumps back to saved address in main
 	 nop
 	 
numb:
 	addi $v0,$v0,0x030
 	jr $ra
 	nop	 
delay:
	PUSH $ra		# saves return address
	move $t2, $a0 		# ms

	addi $t0, $zero, 0	# i=0
while:
	ble $t2,$zero, exit	# while loop. if ms <= 0 jump to exit 
	nop
	
	sub $t2, $t2, 1		# ms--
	addi $t1, $t1, 6000	# constant
for:
	addi $t0, $t0, 1	# i++
	ble $t0, $t1, for
	nop
	
	j while
	nop
exit:
	POP $ra
	jr $ra
	nop

	
time2string:
	PUSH $s0	#push to save the registers and return address to the stack in case we need them later
	PUSH $s1
	PUSH $ra
	move $s1, $a1	#$a0 is the timestring and $a1 is the digits making up the time
	move $s0, $a0
	
	# Gets the first digit in the time, i.e X0:00
	andi $t0, $s1, 0xf000 # masks everything before bit 16 and everythiing after bit 13
	srl $a0, $t0, 12	#move the four bits to the end
	jal hexasc		#call the hexasc function the get an ascii value for the digit
	nop
	sb $v0, 0($s0)		#stores the return value from hexasc
	
	#second digit
	andi $t0, $s1, 0x0f00
	srl $a0, $t0, 8
	jal hexasc
	nop
	sb $v0, 1($s0)
	
	#adds the colon
	addi $t1, $zero, 0x3a
	sb $t1, 2($s0)
	
	#third digit
	andi $t0, $a1, 0x00f0
	srl $a0, $t0, 4
	jal hexasc
	nop
	sb $v0, 3($s0)
	
	#fourth digit
	andi $t0, $a1, 0x000f
	move $a0, $t0
	jal hexasc
	nop
	sb $v0, 4($s0)
	
	#adds the null byte
	li $t3, 0x00
	sb $t3, 5($s0)
	
	POP $ra
	POP $s1
	POP $s0
	jr $ra
	nop