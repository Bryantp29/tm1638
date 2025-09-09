#include "stm32f4xx_hal.h"

// Pines asignados
#define TM_STB_GPIO_Port GPIOB
#define TM_STB_Pin       GPIO_PIN_6   // D10

#define TM_CLK_GPIO_Port GPIOC
#define TM_CLK_Pin       GPIO_PIN_7   // D9

#define TM_DIO_GPIO_Port GPIOC
#define TM_DIO_Pin       GPIO_PIN_6   // D8

// Macros para pines
#define STB_HIGH() HAL_GPIO_WritePin(TM_STB_GPIO_Port, TM_STB_Pin, GPIO_PIN_SET)
#define STB_LOW()  HAL_GPIO_WritePin(TM_STB_GPIO_Port, TM_STB_Pin, GPIO_PIN_RESET)

#define CLK_HIGH() HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_SET)
#define CLK_LOW()  HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_RESET)

#define DIO_HIGH() HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_SET)
#define DIO_LOW()  HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_RESET)
#define DIO_READ() HAL_GPIO_ReadPin(TM_DIO_GPIO_Port, TM_DIO_Pin)

// --- Funciones internas ---

// Cambiar pin DIO entre salida y entrada
void DIO_SetOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = TM_DIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TM_DIO_GPIO_Port, &GPIO_InitStruct);
}

void DIO_SetInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = TM_DIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(TM_DIO_GPIO_Port, &GPIO_InitStruct);
}

void TM1638_WriteByte(uint8_t data) {
    DIO_SetOutput();
    for (int i = 0; i < 8; i++) {
        CLK_LOW();
        if (data & 0x01) {
            DIO_HIGH();
        } else {
            DIO_LOW();
        }
        data >>= 1;
        CLK_HIGH();
    }
}

uint8_t TM1638_ReadByte(void) {
    uint8_t data = 0;
    DIO_SetInput();
    for (int i = 0; i < 8; i++) {
        CLK_LOW();
        if (DIO_READ()) {
            data |= (1 << i);
        }
        CLK_HIGH();
    }
    return data;
}

void TM1638_SendCommand(uint8_t cmd) {
    STB_LOW();
    TM1638_WriteByte(cmd);
    STB_HIGH();
}

void TM1638_SetData(uint8_t addr, uint8_t data) {
    TM1638_SendCommand(0x44); // modo dirección fija
    STB_LOW();
    TM1638_WriteByte(0xC0 | addr);
    TM1638_WriteByte(data);
    STB_HIGH();
}

// Tabla de dígitos 0–9 para 7 segmentos
uint8_t digits[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F  // 9
};

void TM1638_Init(void) {
    TM1638_SendCommand(0x8F); // display ON, brillo máximo
}

void TM1638_DisplayTest(void) {
    for (int i = 0; i < 8; i++) {
        TM1638_SetData(i << 1, digits[i % 10]); // muestra 0–7
    }
}

uint8_t TM1638_ReadButtons(void) {
    uint8_t keys = 0;
    STB_LOW();
    TM1638_WriteByte(0x42); // comando de lectura
    for (int i = 0; i < 4; i++) {
        uint8_t v = TM1638_ReadByte();
        keys |= (v << (i * 2)); // cada byte tiene 2 teclas
    }
    STB_HIGH();
    return keys;
}

// --- Programa principal ---
int main(void) {
    HAL_Init();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configura STB y CLK como salida
    GPIO_InitStruct.Pin = TM_STB_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TM_STB_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = TM_CLK_Pin;
    HAL_GPIO_Init(TM_CLK_GPIO_Port, &GPIO_InitStruct);

    // DIO se configurará dinámicamente (entrada/salida)

    // Inicializa TM1638
    TM1638_Init();
    TM1638_DisplayTest();

    while (1) {
        uint8_t keys = TM1638_ReadButtons();

        // Si se presiona un botón, mostrar su índice en el primer dígito
        if (keys) {
            for (int i = 0; i < 8; i++) {
                if (keys & (1 << i)) {
                    TM1638_SetData(0, digits[i % 10]); // mostrar número de botón
                }
            }
        }

        HAL_Delay(100);
    }
}
