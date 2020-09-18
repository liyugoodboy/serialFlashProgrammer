/*****************************************************************
 * 文件名称：mcu_clb.c
 * 文件说明：CLB模块驱动文件
 * 使用说明：
 *    版本：V1.0.0
 * 修改记录：
 * 创建时间：2020.6.17
 *****************************************************************/
#include "driverlib.h"
#include "mcu_clb.h"
#include "math.h"
/******************************************************************
 *函数名称：MCU_CLB_init
 *函数描述：模块初始化
 *输入参数：
 *        base          uint32_t                   CLB模块
 *        initTILE      void(*initTILE)(uint32_t)  syscfg生成的配置函数
 *返回值：
*******************************************************************/
void MCU_CLB_init(uint32_t base,void(*initTILE)(uint32_t))
{
    //使能指定CLB模块
    CLB_enableCLB(base);
    //初始化syscfg中设置内容
    initTILE(base);
    //清除软件输入寄存器
    CLB_setGPREG(base, 0);
    //初始化输入
    CLB_configLocalInputMux(base, CLB_IN0, CLB_LOCAL_IN_MUX_GLOBAL_IN);
    CLB_configLocalInputMux(base, CLB_IN1, CLB_LOCAL_IN_MUX_GLOBAL_IN);
    CLB_configLocalInputMux(base, CLB_IN2, CLB_LOCAL_IN_MUX_GLOBAL_IN);
    CLB_configLocalInputMux(base, CLB_IN3, CLB_LOCAL_IN_MUX_GLOBAL_IN);
    CLB_configLocalInputMux(base, CLB_IN4, CLB_LOCAL_IN_MUX_GLOBAL_IN);
    CLB_configLocalInputMux(base, CLB_IN5, CLB_LOCAL_IN_MUX_GLOBAL_IN);
    CLB_configLocalInputMux(base, CLB_IN6, CLB_LOCAL_IN_MUX_GLOBAL_IN);
    CLB_configLocalInputMux(base, CLB_IN7, CLB_LOCAL_IN_MUX_GLOBAL_IN);
    CLB_configGlobalInputMux(base, CLB_IN0, CLB_GLOBAL_IN_MUX_EPWM1A);
    CLB_configGlobalInputMux(base, CLB_IN1, CLB_GLOBAL_IN_MUX_EPWM1A);
    CLB_configGlobalInputMux(base, CLB_IN2, CLB_GLOBAL_IN_MUX_EPWM1A);
    CLB_configGlobalInputMux(base, CLB_IN3, CLB_GLOBAL_IN_MUX_EPWM1A);
    CLB_configGlobalInputMux(base, CLB_IN4, CLB_GLOBAL_IN_MUX_EPWM1A);
    CLB_configGlobalInputMux(base, CLB_IN5, CLB_GLOBAL_IN_MUX_EPWM1A);
    CLB_configGlobalInputMux(base, CLB_IN6, CLB_GLOBAL_IN_MUX_EPWM1A);
    CLB_configGlobalInputMux(base, CLB_IN7, CLB_GLOBAL_IN_MUX_EPWM1A);
    CLB_configGPInputMux(base, CLB_IN0, CLB_GP_IN_MUX_GP_REG);
    CLB_configGPInputMux(base, CLB_IN1, CLB_GP_IN_MUX_GP_REG);
    CLB_configGPInputMux(base, CLB_IN2, CLB_GP_IN_MUX_GP_REG);
    CLB_configGPInputMux(base, CLB_IN3, CLB_GP_IN_MUX_GP_REG);
    CLB_configGPInputMux(base, CLB_IN4, CLB_GP_IN_MUX_GP_REG);
    CLB_configGPInputMux(base, CLB_IN5, CLB_GP_IN_MUX_GP_REG);
    CLB_configGPInputMux(base, CLB_IN6, CLB_GP_IN_MUX_GP_REG);
    CLB_configGPInputMux(base, CLB_IN7, CLB_GP_IN_MUX_GP_REG);
    CLB_clearInterruptTag(base);
}
/******************************************************************
 *函数名称：MCU_CLB_setInputDataBySw
 *函数描述：软件设置输入通道值(通道必须设置为使用GP寄存器值)
 *输入参数：
 *        base          uint32_t       CLB模块
 *        initTILE      CLB_Inputs     CLB输入通道编号
 *        value         bool           输入通道值
 *返回值：
*******************************************************************/
void MCU_CLB_setInputDataBySw(uint32_t base,CLB_Inputs inID,bool value)
{
    uint8_t gpRegVal;
    gpRegVal = HWREG(base + CLB_LOGICCTL + CLB_O_GP_REG);
    if(value == true)
    {
        gpRegVal |= 0x01 << inID;
    }
    else
    {
        gpRegVal &= ~(0x01 << inID);
    }
    CLB_setGPREG(base, gpRegVal);
}
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
void MCU_CLB_setGpioInput(uint32_t base, uint32_t pin, XBAR_InputNum inputXbar,
                          XBAR_AuxSigNum clbXbar, CLB_Inputs clbInput)
{
    uint32_t muxes;
    uint32_t pinConfig;
    uint32_t pinLargeOffset;
    uint32_t pinSmallOffset;
    uint32_t pinBitOffset;
    XBAR_CLBMuxConfig muxConfig;
    CLB_GlobalInputMux globalMuxCfg;
    //计算配置偏移
    pinLargeOffset = (pin / 32) * 0x400000;
    pinSmallOffset = ((pin % 32) / 16) * 0x20000;
    pinBitOffset = (pin % 16) * 0x200;
    pinConfig = GPIO_0_GPIO0 + pinLargeOffset + pinSmallOffset + pinBitOffset;
    //计算复用信息
    muxConfig = (XBAR_CLBMuxConfig)(XBAR_CLB_MUX01_INPUTXBAR1 + inputXbar * 0x400);
    muxes = XBAR_MUX01 + inputXbar * 4;
    globalMuxCfg = (CLB_GlobalInputMux)(CLB_GLOBAL_IN_MUX_CLB_AUXSIG0 + clbXbar / 2);
    //配置GPIO为输入模式
    GPIO_setPinConfig(pinConfig);
    GPIO_setDirectionMode(pin, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(pin, GPIO_PIN_TYPE_PULLUP);
    //设置GPIO输入连接 INPUT XBAR模块
    XBAR_setInputPin(inputXbar, pin);
    //设置INPUT XBAR 连接 CLB XBAR
    XBAR_setCLBMuxConfig(clbXbar, muxConfig);
    XBAR_enableCLBMux(clbXbar, muxes);
    //设置全局信号复用
    CLB_configGlobalInputMux(base, clbInput, globalMuxCfg);
    //选择全局信号/本地信号
    CLB_configLocalInputMux(base, clbInput, CLB_LOCAL_IN_MUX_GLOBAL_IN);
    //设置CLB输入为外部信号
    CLB_configGPInputMux(base, clbInput, CLB_GP_IN_MUX_EXTERNAL);
}
/******************************************************************
 *函数名称：MCU_CLB_setGpioOutput
 *函数描述：设置GPIO为CLB模块的输出
 *函数说明：在使用GPIO作为输入时，输入逻辑为GPIO->INPUT XBAR->CLB XBAR->CLB INPUT
 *       注意CLB输出编号只有CLBOUT4和CLB_OUT5
 *输入参数：
 *        base          uint32_t             CLB模块
 *        gpioCfg       clb_OutputGpioCfg    GPIO OUTPUT XBAR配置信息
 *        clbOutput     CLB_Outputs          CLB输出编号
 *返回值：
*******************************************************************/
void MCU_CLB_setGpioOutput(uint32_t base,clb_OutputGpioCfg gpioCfg,CLB_Outputs clbOutput)
{
    uint32_t pin;
    uint32_t pinLargeOffset;
    uint32_t pinSmallOffset;
    uint32_t pinBitOffset;
    uint32_t pinConfig;
    uint32_t pinMux;
    uint32_t clbBaseOffset;
    uint32_t clbOutOffset;
    uint32_t xbarMuxCfg;
    uint32_t xbarMuxNum;
    XBAR_OutputNum xbarNum;
    XBAR_OutputMuxConfig xbarMux;
    //计算引脚编号
    pin = gpioCfg & 0x00FF;
    //计算引脚复用编号
    pinMux = (gpioCfg >> 8) & 0x000F;
    //计算引脚复用信息
    pinLargeOffset = (pin / 32) * 0x400000;
    pinSmallOffset = ((pin % 32) / 16) * 0x20000;
    pinBitOffset = (pin % 16) * 0x200;
    pinConfig = GPIO_0_GPIO0 + pinLargeOffset + pinSmallOffset + pinBitOffset
            + pinMux;
    //计算OUTPUT XBAR编号
    xbarNum = (XBAR_OutputNum) (((gpioCfg >> 12) - 1) * 2);
    //计算OUTPUT XBAR复用
    clbBaseOffset = (base - CLB1_BASE) / 0x400;
    clbOutOffset = clbOutput - CLB_OUT4;
    xbarMux = (XBAR_OutputMuxConfig) (((clbBaseOffset * 2 + clbOutOffset)
            * 0x400) + XBAR_OUT_MUX01_CLB1_OUT4);
    xbarMuxNum = clbBaseOffset * 4 + clbOutOffset * 2 + 1;
    xbarMuxCfg = pow(2, xbarMuxNum);
    //设置GPIO为输出模式并连接OUTPUT XBAR模块
    GPIO_setPinConfig(pinConfig);
    GPIO_setDirectionMode(pin, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(pin, GPIO_PIN_TYPE_STD);
    //CLB OUTPUT连接到OUTPUT XBAR模块
    XBAR_setOutputMuxConfig(xbarNum, xbarMux);
    XBAR_enableOutputMux(xbarNum, xbarMuxCfg);
    //使能对应CLB OUTPUTx输出
    CLB_setOutputMask(base, 0x01 << clbOutput, true);
}
/******************************************************************
 *函数名称：MCU_CLB_openInterrupt
 *函数描述：开启模块中断
 *输入参数：
 *        base          uint32_t       CLB模块
 *返回值：
*******************************************************************/
void MCU_CLB_openInterrupt(uint32_t base)
{
    uint32_t intNum;
    void(*isr)() = NULL;
    switch (base) {
        case CLB1_BASE:isr = &MCU_CLB_clb1ISR;intNum = INT_CLB1;break;
        case CLB2_BASE:isr = &MCU_CLB_clb2ISR;intNum = INT_CLB2;break;
        case CLB3_BASE:isr = &MCU_CLB_clb3ISR;intNum = INT_CLB3;break;
        case CLB4_BASE:isr = &MCU_CLB_clb4ISR;intNum = INT_CLB4;break;
        default:break;
    }
    Interrupt_register(intNum, isr);
    Interrupt_enable(intNum);
}
/******************************************************************
 *函数名称：MCU_CLB_clb1ISR
 *函数描述：CLB1模块中断函数
*******************************************************************/
__interrupt void MCU_CLB_clb1ISR(void)
{
    uint16_t intNum;
    //获取中断编号
    intNum = CLB_getInterruptTag(CLB1_BASE);
    //中断编号3
    if(intNum == 3)
    {
    }
    //中断编号4
    if(intNum == 4)
    {
    }
    //清除中断编号寄存器
    CLB_clearInterruptTag(CLB1_BASE);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP5);
}
/******************************************************************
 *函数名称：MCU_CLB_clb2ISR
 *函数描述：CLB2模块中断函数
*******************************************************************/
__interrupt void MCU_CLB_clb2ISR(void)
{
    uint16_t intNum;
    //获取中断编号
    intNum = CLB_getInterruptTag(CLB2_BASE);
    //中断编号4
    if(intNum == 4)
    {

    }
    //清除中断编号寄存器
    CLB_clearInterruptTag(CLB2_BASE);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP5);
}
/******************************************************************
 *函数名称：MCU_CLB_clb3ISR
 *函数描述：CLB2模块中断函数
*******************************************************************/
__interrupt void MCU_CLB_clb3ISR(void)
{
    uint16_t intNum;
    //获取中断编号
    intNum = CLB_getInterruptTag(CLB3_BASE);
    //中断编号4
    if(intNum == 4)
    {

    }
    //清除中断编号寄存器
    CLB_clearInterruptTag(CLB3_BASE);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP5);
}
/******************************************************************
 *函数名称：MCU_CLB_clb4ISR
 *函数描述：CLB4模块中断函数
*******************************************************************/
__interrupt void MCU_CLB_clb4ISR(void)
{
    uint16_t intNum;
    //获取中断编号
    intNum = CLB_getInterruptTag(CLB4_BASE);
    //中断编号4
    if(intNum == 4)
    {

    }
    //清除中断编号寄存器
    CLB_clearInterruptTag(CLB4_BASE);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP5);
}
