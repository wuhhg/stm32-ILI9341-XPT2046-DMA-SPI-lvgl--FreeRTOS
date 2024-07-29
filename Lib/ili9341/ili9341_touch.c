/* vim: set ai et ts=4 sw=4: */

#include "stm32f4xx_hal.h"
#include "ili9341_touch.h"

#define READ_X 0x90
#define READ_Y 0xD0

static void ILI9341_TouchSelect() {
    HAL_GPIO_WritePin(ILI9341_TOUCH_CS_GPIO_Port, ILI9341_TOUCH_CS_Pin, GPIO_PIN_RESET);
}

void ILI9341_TouchUnselect() {
    HAL_GPIO_WritePin(ILI9341_TOUCH_CS_GPIO_Port, ILI9341_TOUCH_CS_Pin, GPIO_PIN_SET);
}

bool ILI9341_TouchPressed() {
    return HAL_GPIO_ReadPin(ILI9341_TOUCH_IRQ_GPIO_Port, ILI9341_TOUCH_IRQ_Pin) == GPIO_PIN_RESET;
}

bool ILI9341_TouchGetCoordinates(uint16_t* x, uint16_t* y) {
    static const uint8_t cmd_read_x[] = { READ_X };
    static const uint8_t cmd_read_y[] = { READ_Y };
    static const uint8_t zeroes_tx[] = { 0x00, 0x00 };

    ILI9341_TouchSelect();

    uint32_t avg_x1 = 0;
		uint32_t avg_x2 = 0;
		uint32_t avg_y1 = 0;
    uint32_t avg_y2 = 0;
    uint8_t nsamples = 0;
    for(uint8_t i = 0; i < 16; i++) {
        if(!ILI9341_TouchPressed())
            break;

        nsamples++;

        HAL_SPI_Transmit(&ILI9341_TOUCH_SPI_PORT, (uint8_t*)cmd_read_y, sizeof(cmd_read_y), HAL_MAX_DELAY);
        uint8_t y1_raw[2];
        HAL_SPI_TransmitReceive(&ILI9341_TOUCH_SPI_PORT, (uint8_t*)zeroes_tx, y1_raw, sizeof(y1_raw), HAL_MAX_DELAY);
				
				HAL_SPI_Transmit(&ILI9341_TOUCH_SPI_PORT, (uint8_t*)cmd_read_x, sizeof(cmd_read_x), HAL_MAX_DELAY);
        uint8_t x1_raw[2];
        HAL_SPI_TransmitReceive(&ILI9341_TOUCH_SPI_PORT, (uint8_t*)zeroes_tx, x1_raw, sizeof(x1_raw), HAL_MAX_DELAY);
				
				HAL_SPI_Transmit(&ILI9341_TOUCH_SPI_PORT, (uint8_t*)cmd_read_y, sizeof(cmd_read_y), HAL_MAX_DELAY);
				uint8_t y2_raw[2];
        HAL_SPI_TransmitReceive(&ILI9341_TOUCH_SPI_PORT, (uint8_t*)zeroes_tx, y2_raw, sizeof(y2_raw), HAL_MAX_DELAY);
				
        
				
				HAL_SPI_Transmit(&ILI9341_TOUCH_SPI_PORT, (uint8_t*)cmd_read_x, sizeof(cmd_read_x), HAL_MAX_DELAY);
        uint8_t x2_raw[2];
        HAL_SPI_TransmitReceive(&ILI9341_TOUCH_SPI_PORT, (uint8_t*)zeroes_tx, x2_raw, sizeof(x2_raw), HAL_MAX_DELAY);

        avg_x1 += (((uint16_t)x1_raw[0]) << 8) | ((uint16_t)x1_raw[1]);
        avg_y1 += (((uint16_t)y1_raw[0]) << 8) | ((uint16_t)y1_raw[1]);
				avg_x2 += (((uint16_t)x2_raw[0]) << 8) | ((uint16_t)x2_raw[1]);
        avg_y2 += (((uint16_t)y2_raw[0]) << 8) | ((uint16_t)y2_raw[1]);
    }

    ILI9341_TouchUnselect();

    if(nsamples < 16)
        return false;
		
    uint32_t raw_x1 = (avg_x1 / 16);
    if(raw_x1 < ILI9341_TOUCH_MIN_RAW_X) raw_x1 = ILI9341_TOUCH_MIN_RAW_X;
    if(raw_x1 > ILI9341_TOUCH_MAX_RAW_X) raw_x1 = ILI9341_TOUCH_MAX_RAW_X;
		
		uint32_t raw_x2 = (avg_x2 / 16);
    if(raw_x2 < ILI9341_TOUCH_MIN_RAW_X) raw_x2 = ILI9341_TOUCH_MIN_RAW_X;
    if(raw_x2 > ILI9341_TOUCH_MAX_RAW_X) raw_x2 = ILI9341_TOUCH_MAX_RAW_X;
		
    uint32_t raw_y1 = (avg_y1 / 16);
    if(raw_y1 < ILI9341_TOUCH_MIN_RAW_Y) raw_y1 = ILI9341_TOUCH_MIN_RAW_Y;
    if(raw_y1 > ILI9341_TOUCH_MAX_RAW_Y) raw_y1 = ILI9341_TOUCH_MAX_RAW_Y;
		
		uint32_t raw_y2 = (avg_y2 / 16);
    if(raw_y2 < ILI9341_TOUCH_MIN_RAW_Y) raw_y2 = ILI9341_TOUCH_MIN_RAW_Y;
    if(raw_y2 > ILI9341_TOUCH_MAX_RAW_Y) raw_y2 = ILI9341_TOUCH_MAX_RAW_Y;
		int ERR_RANGE=150;
		uint32_t raw_x;
		uint32_t raw_y;
		if(((raw_x2<=raw_x1&&raw_x1<raw_x2+ERR_RANGE)||(raw_x1<=raw_x2&&raw_x2<raw_x1+ERR_RANGE))//前后两次采样在+-50内
    &&((raw_y2<=raw_y1&&raw_y1<raw_y2+ERR_RANGE)||(raw_y1<=raw_y2&&raw_y2<raw_y1+ERR_RANGE)))
    {
        raw_x=(raw_x1+raw_x2)/2;
        raw_y=(raw_y1+raw_y2)/2;
 
    }else return 0;
    // Uncomment this line to calibrate touchscreen:
    UART_Printf("raw_x = %d, raw_y = %d\r\n", x, y);
		char a[]=sprintf("raw_x = %d, raw_y = %d\r\n", x, y);
		//ILI9341_TouchUnselect();
		//ILI9341_Select();
		
    *x = (raw_x - ILI9341_TOUCH_MIN_RAW_X) * ILI9341_TOUCH_SCALE_X / (ILI9341_TOUCH_MAX_RAW_X - ILI9341_TOUCH_MIN_RAW_X);
    *y = (raw_y - ILI9341_TOUCH_MIN_RAW_Y) * ILI9341_TOUCH_SCALE_Y / (ILI9341_TOUCH_MAX_RAW_Y - ILI9341_TOUCH_MIN_RAW_Y);
		
    return true;
}
