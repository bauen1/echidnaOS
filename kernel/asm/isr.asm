global handler_simple
global handler_code
global handler_irq_pic0
global handler_irq_pic1
global keyboard_isr
global syscall
global cpu_state_esp

extern keyboard_handler

; API calls
extern char_to_stdout
extern char_from_stdin
extern alloc
extern freemem
extern text_clear

section .data

cpu_state_esp                   dd      cpu_state_stack
interrupted_esp                 dd      0

routine_list:
        times 0x20 dd 0
        dd      char_to_stdout          ; 0x20
        dd      char_from_stdin         ; 0x21
        dd      alloc                   ; 0x22
        dd      freemem                 ; 0x23
        dd      text_clear              ; 0x24

align 4
times 0x1000 db 0
cpu_state_stack:

section .text

bits 32

handler_simple:
        iretd

handler_code:
        add esp, 4
        iretd

handler_irq_pic0:
        push eax
        mov al, 0x20    ; acknowledge interrupt to PIC0
        out 0x20, al
        pop eax
        iretd

handler_irq_pic1:
        push eax
        mov al, 0x20    ; acknowledge interrupt to both PICs
        out 0xA0, al
        out 0x20, al
        pop eax
        iretd

keyboard_isr:
        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi
        push ebp
        push ds
        push es
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        xor eax, eax
        in al, 0x60     ; read from keyboard
        push eax
        call keyboard_handler
        add esp, 4
        mov al, 0x20    ; acknowledge interrupt to PIC0
        out 0x20, al
        pop es
        pop ds
        pop ebp
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax
        iretd

syscall:
; ARGS in EAX (call code), ECX, EDX
; return code in EAX/EDX
        push ebx
        push ecx
        push esi
        push edi
        push ebp
        push ds
        push es
        mov bx, 0x10
        mov ds, bx
        mov es, bx
        mov ebx, 4
        mul ebx
        push edx
        push ecx
        call [routine_list+eax]
        add esp, 8
        pop es
        pop ds
        pop ebp
        pop edi
        pop esi
        pop ecx
        pop ebx
        iretd
