/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_SSD1306.h"
#include "fsl_spi.h"
//#include "fsl_Systick_Delay.h"

#include "util.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief OLED buffer */
static uint8_t OLED_Buffer[(OLED_WIDTH * OLED_HEIGHT) / 8];

/*******************************************************************************
 * Code
 ******************************************************************************/

static void OLED_Command (uint8_t Cmd)
{
	spi_transfer_t xfer = {0};

	GPIO_OutputHigh(OLED_CS_PIN);   // CS HIGH
	GPIO_OutputLow(OLED_DC_PIN); // DC LOW = Command
	GPIO_OutputLow(OLED_CS_PIN); // CS LOW = Assert
	/*Start Transfer*/

	xfer.txData = (uint8_t *)&Cmd;
	xfer.rxData = 0;
	xfer.dataSize = sizeof(Cmd);
	xfer.configFlags = kSPI_EndOfTransfer | kSPI_EndOfFrame |  kSPI_ReceiveIgnore;

	/* Transfer data in polling mode. */
	SPI_MasterTransferBlocking(SPI0, &xfer);
	GPIO_OutputHigh(OLED_CS_PIN);   // CS HIGH

}

void OLED_WriteData (uint8_t *Data, size_t datasize)
{
	spi_transfer_t xfer = {0};

	GPIO_OutputHigh(OLED_CS_PIN);   // CS HIGH
	GPIO_OutputHigh(OLED_DC_PIN); // DC HIGH = Data
	GPIO_OutputLow(OLED_CS_PIN); // CS LOW = Assert
	/*Start Transfer*/
	xfer.txData = Data;
	xfer.rxData = 0;
	xfer.dataSize = datasize;
	xfer.configFlags = kSPI_EndOfTransfer | kSPI_EndOfFrame |  kSPI_ReceiveIgnore;
	/* Transfer data in polling mode. */
	SPI_MasterTransferBlocking(SPI0, &xfer);
	GPIO_OutputHigh(OLED_CS_PIN);   // CS HIGH

}

static void OLED_Reset (void)
{

	GPIO_OutputHigh(OLED_RST_PIN);   // RST HIGH
	Delay_us(1);																			     // Delay 1ms
	GPIO_OutputLow(OLED_RST_PIN); // RST LOW = Assert RST
	Delay_us(10);																			 // Delay 10ms
	GPIO_OutputHigh(OLED_RST_PIN);   // RST HIGH = Release RST

}

static void OLED_ConfigDisplay (void)
{

	OLED_Reset();

	// Turn the OLED Display off
	OLED_Command(OLED_DISPLAYOFF);

	// Configure the display for 128x64 pixels, KS0108 mode
	OLED_Command(OLED_SETDISPLAYCLOCKDIV);
	OLED_Command(0x80);
	OLED_Command(OLED_SETMULTIPLEX);
	OLED_Command(OLED_HEIGHT-1);				// LCD Height
	OLED_Command(OLED_SETDISPLAYOFFSET);
	OLED_Command(0x0);
	OLED_Command(OLED_SETSTARTLINE | 0x0);
	OLED_Command(OLED_CHARGEPUMP);
	OLED_Command(0x14);							// Use 3.3V supply to generate high voltage supply
	OLED_Command(OLED_MEMORYMODE);
	OLED_Command(0x00);
	OLED_Command(OLED_SEGREMAP | 0x1);
	OLED_Command(OLED_COMSCANDEC);
	OLED_Command(OLED_SETCOMPINS);
	OLED_Command(0x12);
	OLED_Command(OLED_SETCONTRAST);
	OLED_Command(0xCF);
	OLED_Command(OLED_SETPRECHARGE);
	OLED_Command(0xF1);
	OLED_Command(OLED_SETVCOMDETECT);
	OLED_Command(0x40);
	OLED_Command(OLED_DISPLAYALLON_RESUME);
//	OLED_Command(OLED_INVERTDISPLAY);
	OLED_Command(OLED_NORMALDISPLAY);
	OLED_Command(OLED_DEACTIVATE_SCROLL);

	OLED_Command(OLED_COLUMNADDR);
	OLED_Command(0);
	OLED_Command(OLED_WIDTH-1);
	OLED_Command(OLED_PAGEADDR);
	OLED_Command(0);
	OLED_Command(OLED_HEIGHT/8-1);

	// Turn the OLED display on!
	OLED_Command(OLED_DISPLAYON);

	OLED_Command(OLED_SETLOWCOLUMN | 0x0);  // low col = 0
	OLED_Command(OLED_SETHIGHCOLUMN | 0x0);  // hi col = 0
	OLED_Command(OLED_SETSTARTLINE | 0x0); // line #0

	OLED_Fill(0);
    OLED_Refresh();
}


static int OLED_Render_Char (uint8_t X_axis, uint8_t Y_axis, uint8_t SC, int8_t String, uint8_t Scale)
{

	uint8_t px, py;
	uint16_t start_pos;

	if ((X_axis >= OLED_WIDTH) || (Y_axis >= OLED_HEIGHT))
	{
		return 1;
	}
	if (String > 127)
	{
		return 2;
	}
	if (Scale > 3)
	{
		return 3;
	}

	start_pos = ((uint8_t)String) * 7;			// Characters have a 7 row offset
	for (px=0; px<5; px++) {
		for (py=0; py<7; py++) {
			if ((font5x7[start_pos+py] >> (7-px)) & 1) {
				switch (Scale) {
				case 3:
					OLED_SetPixel(X_axis+(px*Scale),   Y_axis+(py*Scale),  SC);
					OLED_SetPixel(X_axis+(px*Scale)+1, Y_axis+(py*Scale),  SC);
					OLED_SetPixel(X_axis+(px*Scale)+2, Y_axis+(py*Scale),  SC);
					OLED_SetPixel(X_axis+(px*Scale),   Y_axis+(py*Scale)+1, SC);
					OLED_SetPixel(X_axis+(px*Scale)+1, Y_axis+(py*Scale)+1, SC);
					OLED_SetPixel(X_axis+(px*Scale)+2, Y_axis+(py*Scale)+1, SC);
					OLED_SetPixel(X_axis+(px*Scale),   Y_axis+(py*Scale)+2, SC);
					OLED_SetPixel(X_axis+(px*Scale)+1, Y_axis+(py*Scale)+2, SC);
					OLED_SetPixel(X_axis+(px*Scale)+2, Y_axis+(py*Scale)+2, SC);
					break;
				case 2:
					OLED_SetPixel(X_axis+(px*Scale),   Y_axis+(py*Scale),  SC);
					OLED_SetPixel(X_axis+(px*Scale)+1, Y_axis+(py*Scale),  SC);
					OLED_SetPixel(X_axis+(px*Scale),   Y_axis+(py*Scale)+1, SC);
					OLED_SetPixel(X_axis+(px*Scale)+1, Y_axis+(py*Scale)+1, SC);
					break;
				case 1:
				default:
					OLED_SetPixel(X_axis+px, Y_axis+py, SC);
					break;
				}
			}
		}
	}
	return 0;
}

void OLED_Init(void)
{
	GPIO_PinConfig			(OLED_DC_PIN, gpio_Output_ON);
	GPIO_PinConfig			(OLED_RST_PIN, gpio_Output_ON);
	GPIO_PinConfig			(OLED_CS_PIN, gpio_Output_ON);

	OLED_Reset();
	OLED_Clear();
	OLED_ConfigDisplay();

}

void OLED_Refresh(void)
{
    uint8_t i;
    for(i = 0; i < 8; i++)
    {
    	OLED_Command(0xB0 + i);
    	OLED_Command(OLED_SETLOWCOLUMN);
    	OLED_Command(OLED_SETHIGHCOLUMN);
        OLED_WriteData(&OLED_Buffer[OLED_WIDTH*i],OLED_WIDTH);
    }
}

void OLED_Clear(void)
{
	memset(OLED_Buffer, 0, sizeof(OLED_Buffer));
}

void OLED_Fill(uint8_t Pattern)
{
	memset(OLED_Buffer, Pattern, sizeof(OLED_Buffer));
}

void OLED_DisplayMode (uint8_t Mode)
{
	if (Mode)
	{
		OLED_Command(OLED_INVERTDISPLAY);
	}
	else {
		OLED_Command(OLED_NORMALDISPLAY);
	}
}

void OLED_SetPixel (uint8_t X_axis, uint8_t Y_axis, uint8_t SC)
{
	if((X_axis >= OLED_WIDTH) || (Y_axis >= OLED_HEIGHT))
	{
		//Do nothing
	}
	else
	{
		switch(SC)
		{
			case kOLED_Pixel_Clear:
				OLED_Buffer [X_axis + (Y_axis / 8) * OLED_WIDTH] &= ~(1 << (Y_axis & 7));
				break;
			case kOLED_Pixel_Set:
				OLED_Buffer [X_axis + (Y_axis / 8) * OLED_WIDTH] |= (1 << (Y_axis & 7));
				break;
		}
	}
}

void OLED_SetText (uint8_t X_axis, uint8_t Y_axis, uint8_t SC, char* String, uint8_t Scale)
{
	uint16_t Cont;
	uint16_t xscaled;

	if ((X_axis+2 >= OLED_WIDTH) || (Y_axis >= OLED_HEIGHT)) {
		//Do nothing
	}
	else
	{
		if (Scale > 3) {
			//Do nothing
		}
		else
		{
			for (Cont = 0; String[Cont] != '\0'; Cont++)
			{
				// Catch overflow when scaling!
				xscaled = X_axis+2+(Cont*5*Scale);
				if (xscaled > OLED_WIDTH)
				{
					//Do nothing
				}
				else
				{
					OLED_Render_Char(xscaled, Y_axis, SC, String[Cont], Scale);
				}
			}
		}
	}
}

void OLED_CopyImage(const uint8_t *Img, uint16_t size){

	uint16_t CopyBuffer;

	OLED_Clear();

	for(CopyBuffer = 0; CopyBuffer < size - 1; CopyBuffer++)
	{
		OLED_Buffer[CopyBuffer] = *(Img + CopyBuffer);
	}
}

void OLED_FillRect (uint8_t X_axis, uint8_t Y_axis, uint8_t Width, uint8_t Height)
{
	for (int i = X_axis+2; i < (X_axis+2+Width); i++)
	{
		for (int j=Y_axis; j < Y_axis+Height; j++)
		{
			OLED_SetPixel (i, j, kOLED_Pixel_Set);
		}
	}
}

void OLED_DrawFrame (void)
{
    OLED_FillRect(0, 15, OLED_WIDTH, 1);
    OLED_FillRect(0, 30, OLED_WIDTH, 1);
    OLED_FillRect(29, 0, 1, 30);
    OLED_FillRect(64, 0, 1, 30);
    OLED_FillRect(99, 0, 1, 30);
}
