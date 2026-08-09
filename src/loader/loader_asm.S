/* loader_asm.S — setjmp/longjmp загрузчика .BIN-программ.
 *
 * Отдельный ассемблерный файл: кадр стека фиксирован и не зависит
 * от пролога, который компилятор вставляет в C-функцию.
 *
 * Формат контекста (uint32_t env[6]):
 *   [0] = ebx  [1] = esi  [2] = edi  [3] = ebp  [4] = esp  [5] = eip
 */

.intel_syntax noprefix

.globl loader_setjmp
.type loader_setjmp, @function
/* вход: [esp]=ret, [esp+4]=env */
loader_setjmp:
    push ebp                     /* [esp]=old ebp, [esp+4]=ret, [esp+8]=env */
    mov  ecx, [esp+8]            /* env */
    mov  [ecx+0], ebx
    mov  [ecx+4], esi
    mov  [ecx+8], edi
    mov  [ecx+12], ebp           /* входной ebp (кадр вызывающего) */
    lea  edx, [esp+8]            /* esp вызывающего после ret */
    mov  [ecx+16], edx
    mov  edx, [esp+4]            /* адрес возврата */
    mov  [ecx+20], edx
    xor  eax, eax                /* setjmp возвращает 0 */
    pop  ebp
    ret

.globl loader_longjmp
.type loader_longjmp, @function
/* вход: [esp]=ret, [esp+4]=env, [esp+8]=value */
loader_longjmp:
    mov  eax, [esp+8]            /* value */
    test eax, eax
    jnz  1f
    mov  eax, 1                  /* longjmp(env, 0) => 1 (setjmp-семантика) */
1:
    mov  ecx, [esp+4]            /* env */
    mov  ebx, [ecx+0]
    mov  esi, [ecx+4]
    mov  edi, [ecx+8]
    mov  ebp, [ecx+12]
    mov  esp, [ecx+16]
    jmp  [ecx+20]                /* не возвращается */
