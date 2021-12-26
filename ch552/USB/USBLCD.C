
/********************************** (C) COPYRIGHT *******************************
* File Name          : VendorDefinedDev.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/01/20
* Description        : CH554模拟USB Module(CH554),厂商自定义接口设备，需要安装驱动，
驱动搜索CH37XDRV或者安装ISPTool会自动安装该设备类驱动，该设备类除了控制传输外，还是直插端点2批量上下传和端点1
中断上传，可以通过372DEBUG.EXE获取其他USB调试工具进行收发数据演示
            
*******************************************************************************/

#define DEFAULT_ENDP0_SIZE 64
#include "./Public/CH554.H"
#include "./Public/Debug.H"
#include "GPIO.H"
#include "LCD.H"
#include "VM.H"
#include <stdio.h>
#include <string.h>

#define THIS_ENDP0_SIZE DEFAULT_ENDP0_SIZE
#define ENDP2_IN_SIZE 64
#define ENDP2_OUT_SIZE 64

#define LCD_COLS 16
#define LCD_ROWS 2
#define OLED 1

// 设备描述符
UINT8C MyDevDescr[] = {0x12, 0x01, 0x10, 0x01,
					   0xFF, 0x80, 0x55, THIS_ENDP0_SIZE,
					   0x62, 0x13, 0x32, 0x55, // 厂商ID和产品ID
					   0x00, 0x01, 0x01, 0x02,
					   0x00, 0x01};
// 配置描述符
UINT8C MyCfgDescr[] = {0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0x80, 0x32,
					   0x09, 0x04, 0x00, 0x00, 0x02, 0x06, 0x80, 0x55, 0x00,
					   0x07, 0x05, 0x82, 0x02, ENDP2_IN_SIZE, 0x00, 0x00,
					   0x07, 0x05, 0x02, 0x02, ENDP2_OUT_SIZE, 0x00, 0x00};
// 语言描述符
UINT8C MyLangDescr[] = {0x04, 0x03, 0x09, 0x04};
// 厂家信息
UINT8C MyManuInfo[] = {0x0E, 0x03, 'U', 0, 'S', 0, 'B', 0, 'L', 0, 'C', 0, 'D', 0};
// 产品信息
UINT8C MyProdInfo[] = {0x0C, 0x03, '1', 0, '6', 0, '0', 0, '2', 0, 'E', 0};

UINT8 UsbConfig = 0; // USB配置标志

UINT8C DevInfo[] = {LCD_ROWS, LCD_COLS, OLED};

UINT8X Ep0Buffer[MIN(64, THIS_ENDP0_SIZE + 2)] _at_ 0x0000;													   // OUT&IN
UINT8X Ep2Buffer[MIN(64, ENDP2_IN_SIZE + 2) + MIN(64, ENDP2_OUT_SIZE + 2)] _at_(MIN(64, THIS_ENDP0_SIZE + 2)); // OUT+IN

#define UsbSetupBuf ((PUSB_SETUP_REQ)Ep0Buffer)

#pragma NOAREGS

// #define FIFO_SIZE 64

// UINT8 ins_fifo[FIFO_SIZE];
// UINT8 fifo_start = 0;
// UINT8 fifo_end = 0;

// // 返回读取长度
// UINT8 fifo_read(UINT8 *p, UINT8 max_len)
// {
// 	UINT8 len1 = 0;
// 	UINT8 len2 = 0;
// 	UINT8 _fifo_end = fifo_end;
// 	if (_fifo_end == fifo_start)
// 	{
// 		return 0;
// 	}
// 	if (_fifo_end > fifo_start)
// 	{
// 		len1 = MIN(max_len, _fifo_end - fifo_start);
// 		memcpy(p, &ins_fifo[fifo_start], len1);
// 		fifo_start += len1;
// 		return len1;
// 	}
// 	else
// 	{
// 		len1 = MIN(max_len, FIFO_SIZE - fifo_start);
// 		memcpy(p, &ins_fifo[fifo_start], len1);
// 		if (len1 < max_len)
// 		{
// 			p = &p[len1];
// 			len2 = MIN(max_len - len1, _fifo_end);
// 			memcpy(p, &ins_fifo[0], len2);
// 			fifo_start = len2;
// 			return len1 + len2;
// 		}
// 		fifo_start += len1;
// 		return len1;
// 	}
// }

// // 返回写入长度
// UINT8 fifo_write(UINT8 *p, UINT8 w_len)
// {
// 	UINT8 len1 = 0;
// 	UINT8 len2 = 0;
// 	UINT8 _fifo_start = fifo_start;
// 	if (_fifo_start == (fifo_end + 1) % FIFO_SIZE)
// 	{
// 		// 已经满
// 		return 0;
// 	}
// 	if (fifo_end < _fifo_start)
// 	{
// 		len1 = MIN(w_len, _fifo_start - fifo_end - 1);
// 		memcpy(&ins_fifo[fifo_end], p, len1);
// 		fifo_end += len1;
// 		return len1;
// 	}
// 	else
// 	{
// 		len1 = MIN(w_len, FIFO_SIZE - fifo_end);
// 		memcpy(&ins_fifo[fifo_end], p, len1);
// 		if (len1 < w_len && _fifo_start > 1)
// 		{
// 			p = &p[len1];
// 			len2 = MIN(w_len - len1, _fifo_start - 1);
// 			memcpy(&ins_fifo[0], p, len2);
// 			fifo_end = len2;
// 			return len1 + len2;
// 		}
// 		fifo_end += len1;
// 		return len1;
// 	}
// }

// UINT8C LCD_Code[] = {
// 	// backlight
// 	MAKE_INS(OP_SET_IO, 1),
// 	// delay 100ms
// 	MAKE_INS(OP_DELAY_MS, 4),
// 	100,
// 	MAKE_INS(OP_CMD_4, 0x03),
// 	// delay 4.1ms
// 	OP_DELAY_MS,
// 	4,
// 	OP_DELAY_US,
// 	200,
// 	MAKE_INS(OP_CMD_4, 0x03),
// 	// delay 4.1ms
// 	OP_DELAY_MS,
// 	4,
// 	OP_DELAY_US,
// 	200,
// 	MAKE_INS(OP_CMD_4, 0x03),
// 	OP_DELAY_US,
// 	150,
// 	// 4bit mode
// 	MAKE_INS(OP_CMD_4, 0x02),
// 	// set function and display
// 	MAKE_INS(OP_CMD_8, 2),
// 	LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS,
// 	LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKOFF,
// 	LCD_CLEARDISPLAY,
// 	OP_DELAY_MS,
// 	2,
// 	// set display mode
// 	MAKE_INS(OP_CMD_8, 2),
// 	LCD_SETDDRAMADDR | (0),
// 	LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT,
// 	LCD_RETURNHOME,
// 	OP_DELAY_MS,
// 	2,
// 	MAKE_INS(OP_DAT_8, 4),
// 	'H',
// 	'e',
// 	'l',
// 	'l',
// 	'o',
// };

void USB_SendDevInfo()
{
	UINT8 i;
	for (i = 0; i < sizeof(DevInfo); i++)
	{
		Ep2Buffer[MAX_PACKET_SIZE + i] = DevInfo[i];
	}
	UEP2_T_LEN = sizeof(DevInfo);
	UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // 允许上传
}

void USB_DeviceInterrupt(void) interrupt INT_NO_USB using 1 /* USB中断服务程序,使用寄存器组1 */
{
	int len, i;
	static UINT8 SetupReqCode, SetupLen;
	static PUINT8 pDescr;
	if (UIF_TRANSFER)
	{ // USB传输完成
		if (U_IS_NAK)
		{
		}
		else
		{
			switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
			{						// 分析操作令牌和端点号
			case UIS_TOKEN_OUT | 2: // endpoint 2# 批量端点下传
				if (U_TOG_OK)
				{							 // 不同步的数据包将丢弃
					UEP2_CTRL ^= bUEP_R_TOG; // 手动翻转
					len = USB_RX_LEN;
					for (i = 0; i < len; i++)
					{
						VM_Execute(Ep2Buffer[i]);
					}
				}
				break;
			case UIS_TOKEN_IN | 2:										 // endpoint 2# 批量端点上传
				UEP2_CTRL ^= bUEP_T_TOG;								 // 手动翻转
				UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; // 暂停上传
				USB_SendDevInfo();
				break;
			case UIS_TOKEN_SETUP | 0: // endpoint 0# SETUP
				UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
				len = USB_RX_LEN;
				if (len == sizeof(USB_SETUP_REQ))
				{ // SETUP包长度
					SetupLen = UsbSetupBuf->wLengthL;
					if (UsbSetupBuf->wLengthH || SetupLen > 0x7F)
						SetupLen = 0x7F; // 限制总长度
					len = 0;			 // 默认为成功并且上传0长度
					SetupReqCode = UsbSetupBuf->bRequest;
					if ((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD)
					{ /* 非标准请求 */
						len = 0xFF;
					}
					else
					{ // 标准请求
						switch (SetupReqCode)
						{ // 请求码
						case USB_GET_DESCRIPTOR:
							switch (UsbSetupBuf->wValueH)
							{
							case 1: // 设备描述符
								pDescr = (PUINT8)(&MyDevDescr[0]);
								len = sizeof(MyDevDescr);
								break;
							case 2: // 配置描述符
								pDescr = (PUINT8)(&MyCfgDescr[0]);
								len = sizeof(MyCfgDescr);
								break;
							case 3: // 字符串描述符
								switch (UsbSetupBuf->wValueL)
								{
								case 1:
									pDescr = (PUINT8)(&MyManuInfo[0]);
									len = sizeof(MyManuInfo);
									break;
								case 2:
									pDescr = (PUINT8)(&MyProdInfo[0]);
									len = sizeof(MyProdInfo);
									break;
								case 0:
									pDescr = (PUINT8)(&MyLangDescr[0]);
									len = sizeof(MyLangDescr);
									break;
								default:
									len = 0xFF; // 不支持的字符串描述符
									break;
								}
								break;
							default:
								len = 0xFF; // 不支持的描述符类型
								break;
							}
							if (SetupLen > len)
								SetupLen = len;												// 限制总长度
							len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen; // 本次传输长度
							memcpy(Ep0Buffer, pDescr, len);									/* 加载上传数据 */
							SetupLen -= len;
							pDescr += len;
							break;
						case USB_SET_ADDRESS:
							SetupLen = UsbSetupBuf->wValueL; // 暂存USB设备地址
							break;
						case USB_GET_CONFIGURATION:
							Ep0Buffer[0] = UsbConfig;
							if (SetupLen >= 1)
								len = 1;
							break;
						case USB_SET_CONFIGURATION:
							UsbConfig = UsbSetupBuf->wValueL;
							break;
						case USB_CLEAR_FEATURE:
							if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
							{ // 端点
								switch (UsbSetupBuf->wIndexL)
								{
								case 0x82:
									UEP2_CTRL = UEP2_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
									break;
								case 0x02:
									UEP2_CTRL = UEP2_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
									break;
								default:
									len = 0xFF; // 不支持的端点
									break;
								}
							}
							else
								len = 0xFF; // 不是端点不支持
							break;
						case USB_GET_INTERFACE:
							Ep0Buffer[0] = 0x00;
							if (SetupLen >= 1)
								len = 1;
							break;
						case USB_GET_STATUS:
							Ep0Buffer[0] = 0x00;
							Ep0Buffer[1] = 0x00;
							if (SetupLen >= 2)
								len = 2;
							else
								len = SetupLen;
							break;
						default:
							len = 0xFF; // 操作失败
#ifdef DE_PRINTF
							printf("ErrEp0ReqCode=%02X\n", (UINT16)SetupReqCode);
#endif
							break;
						}
					}
				}
				else
				{
					len = 0xFF; // SETUP包长度错误
#ifdef DE_PRINTF
					printf("ErrEp0ReqSize\n");
#endif
				}
				if (len == 0xFF)
				{ // 操作失败
					SetupReqCode = 0xFF;
					UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL; // STALL
				}
				else if (len <= THIS_ENDP0_SIZE)
				{ // 上传数据或者状态阶段返回0长度包
					UEP0_T_LEN = len;
					UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; // 默认数据包是DATA1
				}
				else
				{																		 // 下传数据或其它
					UEP0_T_LEN = 0;														 // 虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
					UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; // 默认数据包是DATA1
				}
				break;
			case UIS_TOKEN_IN | 0: // endpoint 0# IN
				switch (SetupReqCode)
				{
				case USB_GET_DESCRIPTOR:
					len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen; // 本次传输长度
					memcpy(Ep0Buffer, pDescr, len);									/* 加载上传数据 */
					SetupLen -= len;
					pDescr += len;
					UEP0_T_LEN = len;
					UEP0_CTRL ^= bUEP_T_TOG; // 翻转
					break;
				case USB_SET_ADDRESS:
					USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
					UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
					break;
				default:
					UEP0_T_LEN = 0; // 状态阶段完成中断或者是强制上传0长度数据包结束控制传输
					UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
					break;
				}
				break;
			case UIS_TOKEN_OUT | 0: // endpoint 0# OUT
				switch (SetupReqCode)
				{
				case USB_GET_DESCRIPTOR:
				default:
					UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK; // 准备下一控制传输
					break;
				}
				break;
			default:
				break;
			}
		}
		UIF_TRANSFER = 0; // 清中断标志
	}
	else if (UIF_BUS_RST)
	{ // USB总线复位
		UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		USB_DEV_AD = 0x00;
		UIF_SUSPEND = 0;
		UIF_TRANSFER = 0;
		UIF_BUS_RST = 0; // 清中断标志
		printf("reset\n");
		USB_SendDevInfo();
	}
	else if (UIF_SUSPEND)
	{ // USB总线挂起/唤醒完成
		UIF_SUSPEND = 0;
		if (USB_MIS_ST & bUMS_SUSPEND)
		{ // 挂起
#ifdef DE_PRINTF
			printf("zz\n"); // 睡眠状态
#endif
			while (XBUS_AUX & bUART0_TX)
				; // 等待发送完成
			SAFE_MOD = 0x55;
			SAFE_MOD = 0xAA;
			WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO; // USB或者RXD0有信号时可被唤醒
			PCON |= PD;								// 睡眠
			SAFE_MOD = 0x55;
			SAFE_MOD = 0xAA;
			WAKE_CTRL = 0x00;
		}
		else
		{
			// 唤醒
			printf("!!\n"); // 睡眠状态
			USB_SendDevInfo();
		}
	}
	else
	{
		// 意外的中断,不可能发生的情况
		USB_INT_FG = 0xFF; // 清中断标志
	}
}

/*******************************************************************************
* Function Name  : InitUSB_Device()
* Description    : USB设备模式配置,设备模式启动，收发端点配置，中断开启
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void InitUSB_Device(void) // 初始化USB设备
{
	IE_USB = 0;
	USB_CTRL = 0x00;						// 先设定模式
	UEP2_3_MOD = bUEP2_RX_EN | bUEP2_TX_EN; // 端点2下传OUT和上传IN
	UEP0_DMA = Ep0Buffer;
	UEP2_DMA = Ep2Buffer;
	USB_DEV_AD = 0x00;
	UDEV_CTRL = bUD_PD_DIS;								  // 禁止DP/DM下拉电阻
	USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN; // 启动USB设备及DMA，在中断期间中断标志未清除前自动返回NAK
	UDEV_CTRL |= bUD_PORT_EN;							  // 允许USB端口
	USB_INT_FG = 0xFF;									  // 清中断标志
	USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
	IE_USB = 1;
}

main()
{
	CfgFsys();
	mDelaymS(5); //修改主频，稍加延时等待主频稳定
	GpioInit();
	mInitSTDIO(); /* 初始化串口0为了让计算机通过串口监控演示过程 */
				  // for (i = 0; i < sizeof(LCD_Code); i++)
				  // {
				  // 	VM_Execute(LCD_Code[i]);
				  // }
#ifdef DE_PRINTF
	printf("Start @ChipID=%02X\n", (UINT16)CHIP_ID);
#endif
	InitUSB_Device();
	EA = 1;
	UEP2_T_LEN = 0;
	USB_SendDevInfo();

	while (1)
	{
	}
}
