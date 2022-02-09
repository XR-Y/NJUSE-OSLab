; add and mul ±10^20
; 数字存储：数字变量地址=数字最小位，数字变量地址+数字位数-1=数字最大位
section .data
    ; string
    userMsg1 : db "OS_LAB_1  sample big number add & mul", 0ah
    lenUserMsg1 : equ $ - userMsg1
    userMsg2 : db "Please input x and y :", 0ah
    lenUserMsg2 : equ $ - userMsg2
    newLine : db 0ah    ; 换行符
    negative : db 2dh   ; 负号

    ; numbers and results
    input : times 50 db 0
    a : times 24 db 0
    b : times 24 db 0
    a_sign : db 0
    b_sign : db 0
    a_len : dd 0
    b_len : dd 0
    
    res_add : times 25 db 0
    res_add_len : dd 0
    res_add_sign : db 0
    carry : db 0    ; 进位或借位

    res_sub : times 25 db 0
    res_sub_len : dd 0
    res_sub_sign : db 0
    
    res_mul : times 50 db 0
    res_mul_len : dd 0
    res_mul_sign : db 0

    char_num : db 0  ; ecx储存地址，必须开内存地址储存值得以打印

section .text
    global _start
    ; global main   ; debug

_start:
; main:
    call printTips
    mov eax, 3
    mov ebx, 0
    mov ecx, input
    mov edx, 50 ; 读取输入的最大长度
    int 80h
    call readNumber
    ; call printA
    ; call printB
    call add_sub
    call mulAB
    mov eax, 1  ; sys_exit
    int 80h

readNumber: 
    call getA
    call getB
    ret

    getA:
        mov ebx, input
        mov esi, 0
        mov byte al, [ebx + esi]
    ; 将输入读入eax
    a2eax:
        push eax
        inc esi
        mov byte al, [ebx + esi]
        cmp al, ' '
        jnz a2eax ; 直至遇到空格则停止
        mov [a_len], esi
        mov ebx, a
        mov edi, 0
    ; 将eax中的输入存入内存中，从末位（尾端）开始处理，放到低地址（小端存储）
    eax2memA:
        pop eax
        cmp al, '-' ; 读到负号说明已经结束读取，直接设置符号为负后返回
        jz set_a_neg
        sub al, 0x30 ; ascii码处理
        mov [ebx + edi], al
        inc edi
        cmp edi, esi
        jnz eax2memA
        ret
    set_a_neg:
        mov byte[a_sign], 1
        dec esi ; 负号时实际数据长度减一位
        mov [a_len], esi
        ret
    
    getB:
        mov ebx, input
        mov esi, [a_len]
        add ebx, esi
        inc ebx ; 处理输入两数间的空格
        cmp byte[a_sign], 1   ; 考虑负数情况
        jnz isPositive
        add ebx, 0x01
        isPositive:
            mov esi, 0
        mov byte al, [ebx + esi]
    b2eax:
        push eax
        inc esi
        mov byte al, [ebx + esi]
        cmp al, 0ah
        jnz b2eax ; 直至遇到换行则停止
        mov [b_len], esi
        mov ebx, b
        mov edi, 0
    eax2memB:
        pop eax
        cmp al, '-'
        jz set_b_neg
        sub al, 0x30 ;ascii码处理
        mov [ebx + edi], al
        inc edi
        cmp edi, esi
        jnz eax2memB
        ret
    set_b_neg:
        mov byte[b_sign], 1
        dec esi
        mov [b_len], esi
        ret

print:
    normal:
        mov ebx, 1  ; stdout
        mov eax, 4  ; sys_write
        int 80h
        ret
    ; 打印提示信息
    printTips:
        mov edx, lenUserMsg1
        mov ecx, userMsg1
        call normal
        mov edx, lenUserMsg2
        mov ecx, userMsg2
        call normal
        ret
    ; 打印空行
    printLine:
        mov ecx, newLine
        mov edx, 1
        call normal
        ret
    ; 打印符号
    printSign:
        cmp al, 1
        jnz pos_number
        mov ecx, negative
        mov edx, 1
        call normal
        ret
    pos_number: 
        ret
    ; just use to debug ============================================================
    ; printA:
    ;     mov al, [a_sign]
    ;     call printSign
    ;     mov edx, 1
    ;     mov esi, [a_len]
    ; printCharA:
    ;     mov byte cl, [esi + a - 1]
    ;     add byte cl, 0x30
    ;     mov byte[char_num], cl
    ;     mov ecx, char_num
    ;     call normal
    ;     dec esi
    ;     cmp esi, 0
    ;     jnz printCharA
    ;     call printLine
    ;     ret

    ; printB:
    ;     mov al, [b_sign]
    ;     call printSign
    ;     mov edx, 1
    ;     mov esi, [b_len]
    ; printCharB:
    ;     mov byte cl, [esi + b - 1]
    ;     add byte cl, 0x30
    ;     mov byte[char_num], cl
    ;     mov ecx, char_num
    ;     call normal
    ;     dec esi
    ;     cmp esi, 0
    ;     jnz printCharB
    ;     call printLine
    ;     ret
    ; ============================================================================
    ; 小端存储，所有打印数字结果均从高地址开始，esi=0时结束
    printAdd:
        mov edx, 1
        mov esi, [res_add_len]
        cmp esi, 1
        jz printOneAdd ; 只有一位结果则直接打印
    dealWithZeroAdd:
        cmp byte[res_add + esi - 1], 0  ;出现第一位不为0的数则跳转至打印符号
        jnz printNegAdd
        dec esi
        cmp esi, 1
        jz printOneAdd ; 当且仅当只剩一位时需要打印0
        jmp dealWithZeroAdd
    printNegAdd:
        mov al, [res_add_sign]
        call printSign
        jmp printCharAdd
    printCharAdd:
        mov byte cl, [esi + res_add - 1]
        add byte cl, 0x30
        mov byte[char_num], cl
        mov ecx, char_num
        call normal
        dec esi
        cmp esi, 0
        jnz printCharAdd
        call printLine
        ret
    printOneAdd:
        mov byte cl, [esi + res_add - 1]
        cmp cl, 0
        jnz printNegAdd ; 结果为0时不打印负号，避免出现-0
        call printCharAdd
        ret
    
    printSub:
        mov edx, 1
        mov esi, [res_sub_len]
        cmp esi, 1
        jz printOneSub ; 只有一位结果则直接打印
    dealWithZeroSub:
        cmp byte[res_sub + esi - 1], 0
        jnz printNegSub
        dec esi
        cmp esi, 1
        jz printOneSub ; 当且仅当只剩一位时需要打印0
        jmp dealWithZeroSub
    printNegSub:
        mov al, [res_sub_sign]
        call printSign
        jmp printCharSub
    printCharSub:
        mov byte cl, [esi + res_sub - 1]
        add byte cl, 0x30
        mov byte[char_num], cl
        mov ecx, char_num
        call normal
        dec esi
        cmp esi, 0
        jnz printCharSub
        call printLine
        ret
    printOneSub:
        mov byte cl, [esi + res_sub - 1]
        cmp cl, 0
        jnz printNegSub ; 结果为0时不打印负号，避免出现-0
        call printCharSub
        ret
    
    printMul:
        mov edx, 1
        mov esi, [res_mul_len]
        cmp esi, 1
        jz printOneMul ; 当且仅当只剩一位时需要打印0
    dealWithZeroMul:
        cmp byte[res_mul + esi - 1], 0
        jnz printNegMul
        dec esi
        cmp esi, 1
        jz printOneMul ; 当且仅当只剩一位时需要打印0
        jmp dealWithZeroMul
    printNegMul:
        mov al, [res_mul_sign]
        call printSign
        jmp printCharMul
    printCharMul:
        mov byte cl, [esi + res_mul - 1]
        add byte cl, 0x30
        mov byte[char_num], cl
        mov ecx, char_num
        call normal
        dec esi
        cmp esi, 0
        jnz printCharMul
        call printLine
        ret
    printOneMul:
        mov byte cl, [esi + res_mul - 1]
        cmp cl, 0
        jnz printNegMul ; 结果为0时不打印负号，避免出现-0
        call printCharMul
        ret

addAB:
    ; 比较a,b长度从而判断何时结束加法
    mov eax, [a_len]
    mov ebx, [b_len]
    mov [res_add_len], eax
    cmp eax, ebx
    jge aIsBigger   ; a的长度不小于b
    mov [res_add_len], ebx
    aIsBigger:
        mov esi, 0  ; 小端存储
    addOne: 
        mov ebx, a
        mov al, [ebx + esi]
        mov ebx, b
        mov ah, [ebx + esi]
        add al, ah
        add al, [carry]
        mov byte[carry], 0
        cmp al, 10
        jl saveOne  ; 该位结果不大于10，不进位，直接跳转
        sub al, 10
        mov byte[carry], 1  ; 最多进位为1
    saveOne:
        mov ebx, res_add
        mov [ebx + esi], al
        inc esi
        cmp [res_add_len], esi
        jge addOne
        cmp byte[carry], 1  ; 检查进位
        jnz endAdd
        mov byte[ebx + esi], 0x01
        inc esi
    endAdd:
        mov [res_add_len], esi  ;修正结果位数
        ret

subAB:
    mov esi, 0
    mov eax, [a_len]
    mov ebx, [b_len]
    mov [res_sub_len], eax
    cmp eax, ebx
    jge subOne   ; a的长度不小于b
    mov [res_sub_len], ebx
    subOne:
        mov ebx, a
        mov al, [ebx + esi]
        mov ebx, b
        mov ah, [ebx + esi]
        sub al, ah
        sub al, [carry]
        mov byte[carry], 0
        cmp al, 0
        jge saveOneSub
        mov byte[carry], 1  ; 借位
        add al, 10
    saveOneSub:
        mov [res_sub + esi], al
        inc esi
        cmp esi, [res_sub_len]
        jnz subOne
        cmp byte[carry], 0
        jz endSub
        ; 仍旧借位，说明是小减大，改为大减小
        mov byte[carry], 0
        mov byte[res_sub_sign], 1
        call subBA
    endSub:
        ret

subBA:
    mov esi, 0
    mov eax, [a_len]
    mov ebx, [b_len]
    mov [res_sub_len], eax
    cmp eax, ebx
    jge subOneBA   ; a的长度不小于b
    mov [res_sub_len], ebx
    subOneBA:
        mov ebx, b
        mov al, [ebx + esi]
        mov ebx, a
        mov ah, [ebx + esi]
        sub al, ah
        sub al, [carry]
        mov byte[carry], 0
        cmp al, 0
        jge saveOneSubBA
        mov byte[carry], 1  ; 借位
        add al, 10
    saveOneSubBA:
        mov [res_sub + esi], al
        inc esi
        cmp esi, [res_sub_len]
        jnz subOneBA
        ret


add_sub:
    mov al, [a_sign]
    xor al, [b_sign]
    cmp al, 0
    jz adder    ; 同号相加，异号相减
    suber:
        call subAB  ; a绝对值更大：结果符号为0，a正b负正确，a负b正则需要变号；b绝对值更大，结果符号为1，a正b负正确，a负b正需要变号；综上当且仅当a为负需要变号
        cmp byte[a_sign], 1
        jz neg_sign
        call printSub
        ret
    neg_sign:
        xor byte[res_sub_sign], 1
        call printSub
        ret
    adder:
        mov al, [a_sign]
        mov byte[res_add_sign], al
        call addAB
        call printAdd
        ret

mulAB:
    mov eax, [a_len]
    add eax, [b_len]
    mov [res_mul_len], eax
    ; 预先清空edi和esi
    mov edi, 0
    mov esi, 0
    ; 异号相乘则结果符号置为负
    mov al, [a_sign]
    xor al, [b_sign]
    cmp al, 0
    jz bMulEveryA
    mov byte[res_mul_sign], 1
    ; 模拟b的某一位乘a的每一位
    bMulEveryA:
        mov al, [a + edi]
        mov ah, [b + esi]
        mul ah  ; ax = al * ah
        add al, byte[res_mul + edi + esi]
        mov bl, 10
        div bl  ;商al，余数ah
        mov [res_mul + edi + esi], ah   ; 小端存储，商加至高地址，余数存在低地址
        add [res_mul + edi + esi + 1], al
        inc edi
        cmp edi, [a_len]
        jz nextB
        jmp bMulEveryA
    nextB:
        mov edi, 0
        inc esi
        cmp esi, [b_len]
        jz checkFirstTwo
        jmp bMulEveryA
    ; 最后一次乘法后未检验第二位是否超过9
    checkFirstTwo:
        mov esi, [res_mul_len]
        mov al, [res_mul + esi - 2]
        cmp al, 10
        jl endMul
        mov bl, 10
        div bl
        add byte[res_mul + esi - 1], al
        mov byte[res_mul + esi - 2], ah
        ; 可能会存在第一位是0导致长度比真实长度长一位的可能性，如2*3的情况，但在打印输出时不影响，就不用修正长度了
    endMul:
        call printMul
        ret