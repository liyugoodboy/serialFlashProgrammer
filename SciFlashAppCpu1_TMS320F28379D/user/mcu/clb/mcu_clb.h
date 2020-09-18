/*****************************************************************
 * 文件名称：mcu_clb.h
 * 文件说明：CLB模块驱动文件
 *    版本：V1.0.0
 * 修改记录：
 * 创建时间：2020.6.17
 *****************************************************************/
#ifndef MCU_CLB_H_
#define MCU_CLB_H_

//*****************************************************************
// 设置GPIO为CLB模块输出引脚信息配置选择
// MCU_CLB_setGpioOutput().
//*****************************************************************
typedef enum {
CLB_GPIO_2_OUTPUTXBAR1   =  0x1502U,
CLB_GPIO_3_OUTPUTXBAR2   =  0x2203U,
CLB_GPIO_4_OUTPUTXBAR3   =  0x3504U,
CLB_GPIO_5_OUTPUTXBAR3   =  0x3305U,
CLB_GPIO_6_OUTPUTXBAR4   =  0x4206U,
CLB_GPIO_7_OUTPUTXBAR5   =  0x5307U,
CLB_GPIO_9_OUTPUTXBAR6   =  0x6309U,
CLB_GPIO_11_OUTPUTXBAR7  =  0x730BU,
CLB_GPIO_14_OUTPUTXBAR3  =  0x360EU,
CLB_GPIO_15_OUTPUTXBAR4  =  0x460FU,
CLB_GPIO_16_OUTPUTXBAR7  =  0x7310U,
CLB_GPIO_17_OUTPUTXBAR8  =  0x8311U,
CLB_GPIO_24_OUTPUTXBAR1  =  0x1118U,
CLB_GPIO_25_OUTPUTXBAR2  =  0x2119U,
CLB_GPIO_26_OUTPUTXBAR3  =  0x311AU,
CLB_GPIO_27_OUTPUTXBAR4  =  0x411BU,
CLB_GPIO_28_OUTPUTXBAR5  =  0x551CU,
CLB_GPIO_29_OUTPUTXBAR6  =  0x651DU,
CLB_GPIO_30_OUTPUTXBAR7  =  0x751EU,
CLB_GPIO_31_OUTPUTXBAR8  =  0x851FU,
CLB_GPIO_34_OUTPUTXBAR1  =  0x1122U,
CLB_GPIO_37_OUTPUTXBAR2  =  0x2125U,
CLB_GPIO_48_OUTPUTXBAR3  =  0x3130U,
CLB_GPIO_49_OUTPUTXBAR4  =  0x4131U,
CLB_GPIO_58_OUTPUTXBAR1  =  0x153AU,
CLB_GPIO_59_OUTPUTXBAR2  =  0x253BU,
CLB_GPIO_60_OUTPUTXBAR3  =  0x353CU,
CLB_GPIO_61_OUTPUTXBAR4  =  0x453DU,
}clb_OutputGpioCfg;
/******************************************************************
 *函数名称：MCU_CLB_init
 *函数描述：模块初始化
 *输入参数：
 *        base          uint32_t                   CLB模块
 *        initTILE      void(*initTILE)(uint32_t)  syscfg生成的配置函数
 *返回值：
*******************************************************************/
void MCU_CLB_init(uint32_t base,void(*initTILE)(uint32_t));
/******************************************************************
 *函数名称：MCU_CLB_setGpioInput
 *函数描述：设置GPIO为CLB模块的输入
 *函数说明：在使用GPIO作为输入时，输入逻辑为GPIO->INPUT XBAR->CLB XBAR->CLB INPUT
 *       注意INPUT XBAR连接 CLB XBAR的信号只有 INPUT XBAR1-6
 *输入参数：
 *        base          uint32_t        CLB模块
 *        pin           uint32_t        GPIO引脚编号
 *        inputXbar     XBAR_InputNum   INPUT XBAR编号
 *        clbXbar       XBAR_AuxSigNum  CLB XBAR编号
 *        clbInput      CLB_Inputs      CLB输入编号
 *返回值：
*******************************************************************/
extern void MCU_CLB_setGpioInput(uint32_t base,
                                 uint32_t pin,
                                 XBAR_InputNum inputXbar,
                                 XBAR_AuxSigNum clbXbar,
                                 CLB_Inputs clbInput);
/******************************************************************
 *函数名称：MCU_CLB_setGpioOutput
 *函数描述：设置GPIO为CLB模块的输出
 *函数说明：在使用GPIO作为输入时，输入逻辑为GPIO->INPUT XBAR->CLB XBAR->CLB INPUT
 *       注意CLB输出编号只有CLBOUT4和CLB_OUT5
 *输入参数：
 *        base          uint32_t        CLB模块
 *        pin           uint32_t        GPIO引脚编号
 *        clbOutput     CLB_Outputs     CLB输出编号
 *返回值：
*******************************************************************/
extern void MCU_CLB_setGpioOutput(uint32_t base,
                                  clb_OutputGpioCfg gpioCfg,
                                  CLB_Outputs clbOutput);
/******************************************************************
 *函数名称：MCU_CLB_setInputDataBySw
 *函数描述：软件设置输入通道值(通道必须设置为使用GP寄存器值)
 *输入参数：
 *        base          uint32_t       CLB模块
 *        initTILE      CLB_Inputs     CLB输入通道编号
 *        value         bool           输入通道值
 *返回值：
*******************************************************************/
extern void MCU_CLB_setInputDataBySw(uint32_t base,CLB_Inputs inID,bool value);
/******************************************************************
 *函数名称：MCU_CLB_openInterrupt
 *函数描述：开启模块中断
 *输入参数：
 *        base          uint32_t       CLB模块
 *返回值：
*******************************************************************/
extern void MCU_CLB_openInterrupt(uint32_t base);
/******************************************************************
 *函数描述：CLB模块中断函数
*******************************************************************/
__interrupt void MCU_CLB_clb1ISR(void);
__interrupt void MCU_CLB_clb2ISR(void);
__interrupt void MCU_CLB_clb3ISR(void);
__interrupt void MCU_CLB_clb4ISR(void);
#endif /* MCU_CLB_H_ */
