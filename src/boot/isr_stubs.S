/* ============================================================
 *  NeoPRos — обработчики прерываний (stubs), сгенерировано
 *  скриптом: 256 stub'ов по одному на вектор IDT.
 *  Каждый stub кладёт номер вектора в стек и прыгает в
 *  общий isr_common, который сохраняет контекст и вызывает
 *  isr_dispatch на C.
 *  ВАЖНО: isr_common написан в GAS-синтаксисе (AT&T): в LLVM MC
 *  инструкции push/pop сегментных регистров в Intel-синтаксисе
 *  компилируются в 16-битные (pushw/popw с префиксом 0x66),
 *  что сдвигает стек и ломает struct irq_regs.
 * ============================================================ */

.intel_syntax noprefix

.section .text
.global isr_common
.extern isr_dispatch

    .align 16
    isr_stub_0:
        push 0
        jmp isr_common
    .align 16
    isr_stub_1:
        push 1
        jmp isr_common
    .align 16
    isr_stub_2:
        push 2
        jmp isr_common
    .align 16
    isr_stub_3:
        push 3
        jmp isr_common
    .align 16
    isr_stub_4:
        push 4
        jmp isr_common
    .align 16
    isr_stub_5:
        push 5
        jmp isr_common
    .align 16
    isr_stub_6:
        push 6
        jmp isr_common
    .align 16
    isr_stub_7:
        push 7
        jmp isr_common
    .align 16
    isr_stub_8:
        push 8
        jmp isr_common
    .align 16
    isr_stub_9:
        push 9
        jmp isr_common
    .align 16
    isr_stub_10:
        push 10
        jmp isr_common
    .align 16
    isr_stub_11:
        push 11
        jmp isr_common
    .align 16
    isr_stub_12:
        push 12
        jmp isr_common
    .align 16
    isr_stub_13:
        push 13
        jmp isr_common
    .align 16
    isr_stub_14:
        push 14
        jmp isr_common
    .align 16
    isr_stub_15:
        push 15
        jmp isr_common
    .align 16
    isr_stub_16:
        push 16
        jmp isr_common
    .align 16
    isr_stub_17:
        push 17
        jmp isr_common
    .align 16
    isr_stub_18:
        push 18
        jmp isr_common
    .align 16
    isr_stub_19:
        push 19
        jmp isr_common
    .align 16
    isr_stub_20:
        push 20
        jmp isr_common
    .align 16
    isr_stub_21:
        push 21
        jmp isr_common
    .align 16
    isr_stub_22:
        push 22
        jmp isr_common
    .align 16
    isr_stub_23:
        push 23
        jmp isr_common
    .align 16
    isr_stub_24:
        push 24
        jmp isr_common
    .align 16
    isr_stub_25:
        push 25
        jmp isr_common
    .align 16
    isr_stub_26:
        push 26
        jmp isr_common
    .align 16
    isr_stub_27:
        push 27
        jmp isr_common
    .align 16
    isr_stub_28:
        push 28
        jmp isr_common
    .align 16
    isr_stub_29:
        push 29
        jmp isr_common
    .align 16
    isr_stub_30:
        push 30
        jmp isr_common
    .align 16
    isr_stub_31:
        push 31
        jmp isr_common
    .align 16
    isr_stub_32:
        push 32
        jmp isr_common
    .align 16
    isr_stub_33:
        push 33
        jmp isr_common
    .align 16
    isr_stub_34:
        push 34
        jmp isr_common
    .align 16
    isr_stub_35:
        push 35
        jmp isr_common
    .align 16
    isr_stub_36:
        push 36
        jmp isr_common
    .align 16
    isr_stub_37:
        push 37
        jmp isr_common
    .align 16
    isr_stub_38:
        push 38
        jmp isr_common
    .align 16
    isr_stub_39:
        push 39
        jmp isr_common
    .align 16
    isr_stub_40:
        push 40
        jmp isr_common
    .align 16
    isr_stub_41:
        push 41
        jmp isr_common
    .align 16
    isr_stub_42:
        push 42
        jmp isr_common
    .align 16
    isr_stub_43:
        push 43
        jmp isr_common
    .align 16
    isr_stub_44:
        push 44
        jmp isr_common
    .align 16
    isr_stub_45:
        push 45
        jmp isr_common
    .align 16
    isr_stub_46:
        push 46
        jmp isr_common
    .align 16
    isr_stub_47:
        push 47
        jmp isr_common
    .align 16
    isr_stub_48:
        push 48
        jmp isr_common
    .align 16
    isr_stub_49:
        push 49
        jmp isr_common
    .align 16
    isr_stub_50:
        push 50
        jmp isr_common
    .align 16
    isr_stub_51:
        push 51
        jmp isr_common
    .align 16
    isr_stub_52:
        push 52
        jmp isr_common
    .align 16
    isr_stub_53:
        push 53
        jmp isr_common
    .align 16
    isr_stub_54:
        push 54
        jmp isr_common
    .align 16
    isr_stub_55:
        push 55
        jmp isr_common
    .align 16
    isr_stub_56:
        push 56
        jmp isr_common
    .align 16
    isr_stub_57:
        push 57
        jmp isr_common
    .align 16
    isr_stub_58:
        push 58
        jmp isr_common
    .align 16
    isr_stub_59:
        push 59
        jmp isr_common
    .align 16
    isr_stub_60:
        push 60
        jmp isr_common
    .align 16
    isr_stub_61:
        push 61
        jmp isr_common
    .align 16
    isr_stub_62:
        push 62
        jmp isr_common
    .align 16
    isr_stub_63:
        push 63
        jmp isr_common
    .align 16
    isr_stub_64:
        push 64
        jmp isr_common
    .align 16
    isr_stub_65:
        push 65
        jmp isr_common
    .align 16
    isr_stub_66:
        push 66
        jmp isr_common
    .align 16
    isr_stub_67:
        push 67
        jmp isr_common
    .align 16
    isr_stub_68:
        push 68
        jmp isr_common
    .align 16
    isr_stub_69:
        push 69
        jmp isr_common
    .align 16
    isr_stub_70:
        push 70
        jmp isr_common
    .align 16
    isr_stub_71:
        push 71
        jmp isr_common
    .align 16
    isr_stub_72:
        push 72
        jmp isr_common
    .align 16
    isr_stub_73:
        push 73
        jmp isr_common
    .align 16
    isr_stub_74:
        push 74
        jmp isr_common
    .align 16
    isr_stub_75:
        push 75
        jmp isr_common
    .align 16
    isr_stub_76:
        push 76
        jmp isr_common
    .align 16
    isr_stub_77:
        push 77
        jmp isr_common
    .align 16
    isr_stub_78:
        push 78
        jmp isr_common
    .align 16
    isr_stub_79:
        push 79
        jmp isr_common
    .align 16
    isr_stub_80:
        push 80
        jmp isr_common
    .align 16
    isr_stub_81:
        push 81
        jmp isr_common
    .align 16
    isr_stub_82:
        push 82
        jmp isr_common
    .align 16
    isr_stub_83:
        push 83
        jmp isr_common
    .align 16
    isr_stub_84:
        push 84
        jmp isr_common
    .align 16
    isr_stub_85:
        push 85
        jmp isr_common
    .align 16
    isr_stub_86:
        push 86
        jmp isr_common
    .align 16
    isr_stub_87:
        push 87
        jmp isr_common
    .align 16
    isr_stub_88:
        push 88
        jmp isr_common
    .align 16
    isr_stub_89:
        push 89
        jmp isr_common
    .align 16
    isr_stub_90:
        push 90
        jmp isr_common
    .align 16
    isr_stub_91:
        push 91
        jmp isr_common
    .align 16
    isr_stub_92:
        push 92
        jmp isr_common
    .align 16
    isr_stub_93:
        push 93
        jmp isr_common
    .align 16
    isr_stub_94:
        push 94
        jmp isr_common
    .align 16
    isr_stub_95:
        push 95
        jmp isr_common
    .align 16
    isr_stub_96:
        push 96
        jmp isr_common
    .align 16
    isr_stub_97:
        push 97
        jmp isr_common
    .align 16
    isr_stub_98:
        push 98
        jmp isr_common
    .align 16
    isr_stub_99:
        push 99
        jmp isr_common
    .align 16
    isr_stub_100:
        push 100
        jmp isr_common
    .align 16
    isr_stub_101:
        push 101
        jmp isr_common
    .align 16
    isr_stub_102:
        push 102
        jmp isr_common
    .align 16
    isr_stub_103:
        push 103
        jmp isr_common
    .align 16
    isr_stub_104:
        push 104
        jmp isr_common
    .align 16
    isr_stub_105:
        push 105
        jmp isr_common
    .align 16
    isr_stub_106:
        push 106
        jmp isr_common
    .align 16
    isr_stub_107:
        push 107
        jmp isr_common
    .align 16
    isr_stub_108:
        push 108
        jmp isr_common
    .align 16
    isr_stub_109:
        push 109
        jmp isr_common
    .align 16
    isr_stub_110:
        push 110
        jmp isr_common
    .align 16
    isr_stub_111:
        push 111
        jmp isr_common
    .align 16
    isr_stub_112:
        push 112
        jmp isr_common
    .align 16
    isr_stub_113:
        push 113
        jmp isr_common
    .align 16
    isr_stub_114:
        push 114
        jmp isr_common
    .align 16
    isr_stub_115:
        push 115
        jmp isr_common
    .align 16
    isr_stub_116:
        push 116
        jmp isr_common
    .align 16
    isr_stub_117:
        push 117
        jmp isr_common
    .align 16
    isr_stub_118:
        push 118
        jmp isr_common
    .align 16
    isr_stub_119:
        push 119
        jmp isr_common
    .align 16
    isr_stub_120:
        push 120
        jmp isr_common
    .align 16
    isr_stub_121:
        push 121
        jmp isr_common
    .align 16
    isr_stub_122:
        push 122
        jmp isr_common
    .align 16
    isr_stub_123:
        push 123
        jmp isr_common
    .align 16
    isr_stub_124:
        push 124
        jmp isr_common
    .align 16
    isr_stub_125:
        push 125
        jmp isr_common
    .align 16
    isr_stub_126:
        push 126
        jmp isr_common
    .align 16
    isr_stub_127:
        push 127
        jmp isr_common
    .align 16
    isr_stub_128:
        push 128
        jmp isr_common
    .align 16
    isr_stub_129:
        push 129
        jmp isr_common
    .align 16
    isr_stub_130:
        push 130
        jmp isr_common
    .align 16
    isr_stub_131:
        push 131
        jmp isr_common
    .align 16
    isr_stub_132:
        push 132
        jmp isr_common
    .align 16
    isr_stub_133:
        push 133
        jmp isr_common
    .align 16
    isr_stub_134:
        push 134
        jmp isr_common
    .align 16
    isr_stub_135:
        push 135
        jmp isr_common
    .align 16
    isr_stub_136:
        push 136
        jmp isr_common
    .align 16
    isr_stub_137:
        push 137
        jmp isr_common
    .align 16
    isr_stub_138:
        push 138
        jmp isr_common
    .align 16
    isr_stub_139:
        push 139
        jmp isr_common
    .align 16
    isr_stub_140:
        push 140
        jmp isr_common
    .align 16
    isr_stub_141:
        push 141
        jmp isr_common
    .align 16
    isr_stub_142:
        push 142
        jmp isr_common
    .align 16
    isr_stub_143:
        push 143
        jmp isr_common
    .align 16
    isr_stub_144:
        push 144
        jmp isr_common
    .align 16
    isr_stub_145:
        push 145
        jmp isr_common
    .align 16
    isr_stub_146:
        push 146
        jmp isr_common
    .align 16
    isr_stub_147:
        push 147
        jmp isr_common
    .align 16
    isr_stub_148:
        push 148
        jmp isr_common
    .align 16
    isr_stub_149:
        push 149
        jmp isr_common
    .align 16
    isr_stub_150:
        push 150
        jmp isr_common
    .align 16
    isr_stub_151:
        push 151
        jmp isr_common
    .align 16
    isr_stub_152:
        push 152
        jmp isr_common
    .align 16
    isr_stub_153:
        push 153
        jmp isr_common
    .align 16
    isr_stub_154:
        push 154
        jmp isr_common
    .align 16
    isr_stub_155:
        push 155
        jmp isr_common
    .align 16
    isr_stub_156:
        push 156
        jmp isr_common
    .align 16
    isr_stub_157:
        push 157
        jmp isr_common
    .align 16
    isr_stub_158:
        push 158
        jmp isr_common
    .align 16
    isr_stub_159:
        push 159
        jmp isr_common
    .align 16
    isr_stub_160:
        push 160
        jmp isr_common
    .align 16
    isr_stub_161:
        push 161
        jmp isr_common
    .align 16
    isr_stub_162:
        push 162
        jmp isr_common
    .align 16
    isr_stub_163:
        push 163
        jmp isr_common
    .align 16
    isr_stub_164:
        push 164
        jmp isr_common
    .align 16
    isr_stub_165:
        push 165
        jmp isr_common
    .align 16
    isr_stub_166:
        push 166
        jmp isr_common
    .align 16
    isr_stub_167:
        push 167
        jmp isr_common
    .align 16
    isr_stub_168:
        push 168
        jmp isr_common
    .align 16
    isr_stub_169:
        push 169
        jmp isr_common
    .align 16
    isr_stub_170:
        push 170
        jmp isr_common
    .align 16
    isr_stub_171:
        push 171
        jmp isr_common
    .align 16
    isr_stub_172:
        push 172
        jmp isr_common
    .align 16
    isr_stub_173:
        push 173
        jmp isr_common
    .align 16
    isr_stub_174:
        push 174
        jmp isr_common
    .align 16
    isr_stub_175:
        push 175
        jmp isr_common
    .align 16
    isr_stub_176:
        push 176
        jmp isr_common
    .align 16
    isr_stub_177:
        push 177
        jmp isr_common
    .align 16
    isr_stub_178:
        push 178
        jmp isr_common
    .align 16
    isr_stub_179:
        push 179
        jmp isr_common
    .align 16
    isr_stub_180:
        push 180
        jmp isr_common
    .align 16
    isr_stub_181:
        push 181
        jmp isr_common
    .align 16
    isr_stub_182:
        push 182
        jmp isr_common
    .align 16
    isr_stub_183:
        push 183
        jmp isr_common
    .align 16
    isr_stub_184:
        push 184
        jmp isr_common
    .align 16
    isr_stub_185:
        push 185
        jmp isr_common
    .align 16
    isr_stub_186:
        push 186
        jmp isr_common
    .align 16
    isr_stub_187:
        push 187
        jmp isr_common
    .align 16
    isr_stub_188:
        push 188
        jmp isr_common
    .align 16
    isr_stub_189:
        push 189
        jmp isr_common
    .align 16
    isr_stub_190:
        push 190
        jmp isr_common
    .align 16
    isr_stub_191:
        push 191
        jmp isr_common
    .align 16
    isr_stub_192:
        push 192
        jmp isr_common
    .align 16
    isr_stub_193:
        push 193
        jmp isr_common
    .align 16
    isr_stub_194:
        push 194
        jmp isr_common
    .align 16
    isr_stub_195:
        push 195
        jmp isr_common
    .align 16
    isr_stub_196:
        push 196
        jmp isr_common
    .align 16
    isr_stub_197:
        push 197
        jmp isr_common
    .align 16
    isr_stub_198:
        push 198
        jmp isr_common
    .align 16
    isr_stub_199:
        push 199
        jmp isr_common
    .align 16
    isr_stub_200:
        push 200
        jmp isr_common
    .align 16
    isr_stub_201:
        push 201
        jmp isr_common
    .align 16
    isr_stub_202:
        push 202
        jmp isr_common
    .align 16
    isr_stub_203:
        push 203
        jmp isr_common
    .align 16
    isr_stub_204:
        push 204
        jmp isr_common
    .align 16
    isr_stub_205:
        push 205
        jmp isr_common
    .align 16
    isr_stub_206:
        push 206
        jmp isr_common
    .align 16
    isr_stub_207:
        push 207
        jmp isr_common
    .align 16
    isr_stub_208:
        push 208
        jmp isr_common
    .align 16
    isr_stub_209:
        push 209
        jmp isr_common
    .align 16
    isr_stub_210:
        push 210
        jmp isr_common
    .align 16
    isr_stub_211:
        push 211
        jmp isr_common
    .align 16
    isr_stub_212:
        push 212
        jmp isr_common
    .align 16
    isr_stub_213:
        push 213
        jmp isr_common
    .align 16
    isr_stub_214:
        push 214
        jmp isr_common
    .align 16
    isr_stub_215:
        push 215
        jmp isr_common
    .align 16
    isr_stub_216:
        push 216
        jmp isr_common
    .align 16
    isr_stub_217:
        push 217
        jmp isr_common
    .align 16
    isr_stub_218:
        push 218
        jmp isr_common
    .align 16
    isr_stub_219:
        push 219
        jmp isr_common
    .align 16
    isr_stub_220:
        push 220
        jmp isr_common
    .align 16
    isr_stub_221:
        push 221
        jmp isr_common
    .align 16
    isr_stub_222:
        push 222
        jmp isr_common
    .align 16
    isr_stub_223:
        push 223
        jmp isr_common
    .align 16
    isr_stub_224:
        push 224
        jmp isr_common
    .align 16
    isr_stub_225:
        push 225
        jmp isr_common
    .align 16
    isr_stub_226:
        push 226
        jmp isr_common
    .align 16
    isr_stub_227:
        push 227
        jmp isr_common
    .align 16
    isr_stub_228:
        push 228
        jmp isr_common
    .align 16
    isr_stub_229:
        push 229
        jmp isr_common
    .align 16
    isr_stub_230:
        push 230
        jmp isr_common
    .align 16
    isr_stub_231:
        push 231
        jmp isr_common
    .align 16
    isr_stub_232:
        push 232
        jmp isr_common
    .align 16
    isr_stub_233:
        push 233
        jmp isr_common
    .align 16
    isr_stub_234:
        push 234
        jmp isr_common
    .align 16
    isr_stub_235:
        push 235
        jmp isr_common
    .align 16
    isr_stub_236:
        push 236
        jmp isr_common
    .align 16
    isr_stub_237:
        push 237
        jmp isr_common
    .align 16
    isr_stub_238:
        push 238
        jmp isr_common
    .align 16
    isr_stub_239:
        push 239
        jmp isr_common
    .align 16
    isr_stub_240:
        push 240
        jmp isr_common
    .align 16
    isr_stub_241:
        push 241
        jmp isr_common
    .align 16
    isr_stub_242:
        push 242
        jmp isr_common
    .align 16
    isr_stub_243:
        push 243
        jmp isr_common
    .align 16
    isr_stub_244:
        push 244
        jmp isr_common
    .align 16
    isr_stub_245:
        push 245
        jmp isr_common
    .align 16
    isr_stub_246:
        push 246
        jmp isr_common
    .align 16
    isr_stub_247:
        push 247
        jmp isr_common
    .align 16
    isr_stub_248:
        push 248
        jmp isr_common
    .align 16
    isr_stub_249:
        push 249
        jmp isr_common
    .align 16
    isr_stub_250:
        push 250
        jmp isr_common
    .align 16
    isr_stub_251:
        push 251
        jmp isr_common
    .align 16
    isr_stub_252:
        push 252
        jmp isr_common
    .align 16
    isr_stub_253:
        push 253
        jmp isr_common
    .align 16
    isr_stub_254:
        push 254
        jmp isr_common
    .align 16
    isr_stub_255:
        push 255
        jmp isr_common
/* --- общий обработчик: контекст + вызов C -------------------- */
/* Стек на входе (сверху вниз): [vector] [eip] [cs] [eflags]     */
.att_syntax prefix
isr_common:
    pushal                      /* сохраняем 8 регистров общего назначения */
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    movw $0x10, %ax             /* переключаемся на сегменты ядра */
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    pushl %esp                  /* struct irq_regs * — в isr_dispatch */
    calll isr_dispatch
    addl $4, %esp

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popal
    addl $4, %esp               /* снимаем номер вектора */
    iret

/* --- таблица адресов stub'ов для построения IDT -------------- */
.intel_syntax noprefix
.section .rodata, "a", @progbits
.global isr_stub_table
isr_stub_table:
    .long isr_stub_0
    .long isr_stub_1
    .long isr_stub_2
    .long isr_stub_3
    .long isr_stub_4
    .long isr_stub_5
    .long isr_stub_6
    .long isr_stub_7
    .long isr_stub_8
    .long isr_stub_9
    .long isr_stub_10
    .long isr_stub_11
    .long isr_stub_12
    .long isr_stub_13
    .long isr_stub_14
    .long isr_stub_15
    .long isr_stub_16
    .long isr_stub_17
    .long isr_stub_18
    .long isr_stub_19
    .long isr_stub_20
    .long isr_stub_21
    .long isr_stub_22
    .long isr_stub_23
    .long isr_stub_24
    .long isr_stub_25
    .long isr_stub_26
    .long isr_stub_27
    .long isr_stub_28
    .long isr_stub_29
    .long isr_stub_30
    .long isr_stub_31
    .long isr_stub_32
    .long isr_stub_33
    .long isr_stub_34
    .long isr_stub_35
    .long isr_stub_36
    .long isr_stub_37
    .long isr_stub_38
    .long isr_stub_39
    .long isr_stub_40
    .long isr_stub_41
    .long isr_stub_42
    .long isr_stub_43
    .long isr_stub_44
    .long isr_stub_45
    .long isr_stub_46
    .long isr_stub_47
    .long isr_stub_48
    .long isr_stub_49
    .long isr_stub_50
    .long isr_stub_51
    .long isr_stub_52
    .long isr_stub_53
    .long isr_stub_54
    .long isr_stub_55
    .long isr_stub_56
    .long isr_stub_57
    .long isr_stub_58
    .long isr_stub_59
    .long isr_stub_60
    .long isr_stub_61
    .long isr_stub_62
    .long isr_stub_63
    .long isr_stub_64
    .long isr_stub_65
    .long isr_stub_66
    .long isr_stub_67
    .long isr_stub_68
    .long isr_stub_69
    .long isr_stub_70
    .long isr_stub_71
    .long isr_stub_72
    .long isr_stub_73
    .long isr_stub_74
    .long isr_stub_75
    .long isr_stub_76
    .long isr_stub_77
    .long isr_stub_78
    .long isr_stub_79
    .long isr_stub_80
    .long isr_stub_81
    .long isr_stub_82
    .long isr_stub_83
    .long isr_stub_84
    .long isr_stub_85
    .long isr_stub_86
    .long isr_stub_87
    .long isr_stub_88
    .long isr_stub_89
    .long isr_stub_90
    .long isr_stub_91
    .long isr_stub_92
    .long isr_stub_93
    .long isr_stub_94
    .long isr_stub_95
    .long isr_stub_96
    .long isr_stub_97
    .long isr_stub_98
    .long isr_stub_99
    .long isr_stub_100
    .long isr_stub_101
    .long isr_stub_102
    .long isr_stub_103
    .long isr_stub_104
    .long isr_stub_105
    .long isr_stub_106
    .long isr_stub_107
    .long isr_stub_108
    .long isr_stub_109
    .long isr_stub_110
    .long isr_stub_111
    .long isr_stub_112
    .long isr_stub_113
    .long isr_stub_114
    .long isr_stub_115
    .long isr_stub_116
    .long isr_stub_117
    .long isr_stub_118
    .long isr_stub_119
    .long isr_stub_120
    .long isr_stub_121
    .long isr_stub_122
    .long isr_stub_123
    .long isr_stub_124
    .long isr_stub_125
    .long isr_stub_126
    .long isr_stub_127
    .long isr_stub_128
    .long isr_stub_129
    .long isr_stub_130
    .long isr_stub_131
    .long isr_stub_132
    .long isr_stub_133
    .long isr_stub_134
    .long isr_stub_135
    .long isr_stub_136
    .long isr_stub_137
    .long isr_stub_138
    .long isr_stub_139
    .long isr_stub_140
    .long isr_stub_141
    .long isr_stub_142
    .long isr_stub_143
    .long isr_stub_144
    .long isr_stub_145
    .long isr_stub_146
    .long isr_stub_147
    .long isr_stub_148
    .long isr_stub_149
    .long isr_stub_150
    .long isr_stub_151
    .long isr_stub_152
    .long isr_stub_153
    .long isr_stub_154
    .long isr_stub_155
    .long isr_stub_156
    .long isr_stub_157
    .long isr_stub_158
    .long isr_stub_159
    .long isr_stub_160
    .long isr_stub_161
    .long isr_stub_162
    .long isr_stub_163
    .long isr_stub_164
    .long isr_stub_165
    .long isr_stub_166
    .long isr_stub_167
    .long isr_stub_168
    .long isr_stub_169
    .long isr_stub_170
    .long isr_stub_171
    .long isr_stub_172
    .long isr_stub_173
    .long isr_stub_174
    .long isr_stub_175
    .long isr_stub_176
    .long isr_stub_177
    .long isr_stub_178
    .long isr_stub_179
    .long isr_stub_180
    .long isr_stub_181
    .long isr_stub_182
    .long isr_stub_183
    .long isr_stub_184
    .long isr_stub_185
    .long isr_stub_186
    .long isr_stub_187
    .long isr_stub_188
    .long isr_stub_189
    .long isr_stub_190
    .long isr_stub_191
    .long isr_stub_192
    .long isr_stub_193
    .long isr_stub_194
    .long isr_stub_195
    .long isr_stub_196
    .long isr_stub_197
    .long isr_stub_198
    .long isr_stub_199
    .long isr_stub_200
    .long isr_stub_201
    .long isr_stub_202
    .long isr_stub_203
    .long isr_stub_204
    .long isr_stub_205
    .long isr_stub_206
    .long isr_stub_207
    .long isr_stub_208
    .long isr_stub_209
    .long isr_stub_210
    .long isr_stub_211
    .long isr_stub_212
    .long isr_stub_213
    .long isr_stub_214
    .long isr_stub_215
    .long isr_stub_216
    .long isr_stub_217
    .long isr_stub_218
    .long isr_stub_219
    .long isr_stub_220
    .long isr_stub_221
    .long isr_stub_222
    .long isr_stub_223
    .long isr_stub_224
    .long isr_stub_225
    .long isr_stub_226
    .long isr_stub_227
    .long isr_stub_228
    .long isr_stub_229
    .long isr_stub_230
    .long isr_stub_231
    .long isr_stub_232
    .long isr_stub_233
    .long isr_stub_234
    .long isr_stub_235
    .long isr_stub_236
    .long isr_stub_237
    .long isr_stub_238
    .long isr_stub_239
    .long isr_stub_240
    .long isr_stub_241
    .long isr_stub_242
    .long isr_stub_243
    .long isr_stub_244
    .long isr_stub_245
    .long isr_stub_246
    .long isr_stub_247
    .long isr_stub_248
    .long isr_stub_249
    .long isr_stub_250
    .long isr_stub_251
    .long isr_stub_252
    .long isr_stub_253
    .long isr_stub_254
    .long isr_stub_255
