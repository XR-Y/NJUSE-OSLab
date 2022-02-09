section .data
    red : db 1Bh, '[31;1m', 0
    red_len : equ $ - red
    default_color : db 1Bh, '[37;0m', 0
    default_len : equ $ - default_color

section .text
    global print_red
    global print_data


print_data:
    mov eax, 4
    mov ebx, 1
    mov ecx, [esp + 4]  ; 第一个参数，即字符数组
    mov edx, [esp + 8]  ; 第二个参数，即输出长度
    int 80h
    ret

print_red:
    mov eax, 4
    mov ebx, 1
    mov ecx, red
    mov edx, red_len
    int 80h

    mov eax, 4
    mov ebx, 1
    mov ecx, [esp + 4]
    mov edx, [esp + 8]
    int 80h

    mov eax, 4
    mov ebx, 1
    mov ecx, default_color
    mov edx, default_len
    int 80h
    ret