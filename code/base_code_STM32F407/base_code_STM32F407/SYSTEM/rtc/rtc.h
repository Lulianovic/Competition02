#ifndef __RTC_H
#define __RTC_H	 
#include "sys.h" 

/**
 * @brief  RTC初始化函数
 * @note   配置LSE时钟源、RTC分频系数、默认时间和日期
 * @param  无
 * @retval u8: 0-初始化成功; 1-LSE开启失败; 2-进入初始化模式失败
 */
u8 My_RTC_Init(void);

/**
 * @brief  设置RTC时间
 * @param  hour: 小时 (0-23)
 * @param  min: 分钟 (0-59)
 * @param  sec: 秒钟 (0-59)
 * @param  ampm: 上午/下午标识
 *         @arg RTC_H12_AM: 上午
 *         @arg RTC_H12_PM: 下午
 * @retval ErrorStatus: 成功返回 SUCCESS，失败返回 ERROR
 */
ErrorStatus RTC_Set_Time(u8 hour, u8 min, u8 sec, u8 ampm);

/**
 * @brief  设置RTC日期
 * @param  year: 年份 (0-99)
 * @param  month: 月份 (1-12)
 * @param  date: 日期 (1-31)
 * @param  week: 星期 (1-7, 0为非法值)
 * @retval ErrorStatus: 成功返回 SUCCESS，失败返回 ERROR
 */
ErrorStatus RTC_Set_Date(u8 year, u8 month, u8 date, u8 week);

/**
 * @brief  设置RTC闹钟A (按星期闹铃,24小时制)
 * @param  week: 星期几 (1-7)
 * @param  hour: 小时 (0-23)
 * @param  min: 分钟 (0-59)
 * @param  sec: 秒钟 (0-59)
 * @retval 无
 */
void RTC_Set_AlarmA(u8 week, u8 hour, u8 min, u8 sec);

/**
 * @brief  设置RTC周期性唤醒定时器
 * @param  wksel: 唤醒时钟源选择
 *         @arg RTC_WakeUpClock_RTCCLK_Div16: RTCCLK/16
 *         @arg RTC_WakeUpClock_RTCCLK_Div8: RTCCLK/8
 *         @arg RTC_WakeUpClock_RTCCLK_Div4: RTCCLK/4
 *         @arg RTC_WakeUpClock_RTCCLK_Div2: RTCCLK/2
 *         @arg RTC_WakeUpClock_CK_SPRE_16bits: CK_SPRE (16位)
 *         @arg RTC_WakeUpClock_CK_SPRE_17bits: CK_SPRE (17位)
 * @param  cnt: 自动重装载值 (减到0产生中断)
 * @retval 无
 */
void RTC_Set_WakeUp(u32 wksel, u16 cnt);

#endif