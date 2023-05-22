#ifndef PIN_FUNCTION_H
#define PIN_FUNCTION_H


#define CLEAR_BIT(REG, PIN)                          REG   &= ~( 1UL <<  (PIN))
#define TOGGLE_BIT(REG, PIN)                         REG   ^= (1 <<(PIN))
#define SET_BIT(REG, PIN)                            REG   |= (1UL << (PIN))
#define READ_BIT(REG, PIN)                           ((REG) & (1UL << (PIN)))
#define SET_MULTIPLE_BIT(REG, VALUE, STARTBIT)       REG   |= ((VALUE) << (STARTBIT)) 

#define CLEAR_PIN(REG, PIN)  CLEAR_BIT(REG, PIN)
#define SET_PIN(REG, PIN)    SET_BIT(REG, PIN)
#define READ_PIN(REG, PIN)   READ_BIT(REG, PIN)



#endif