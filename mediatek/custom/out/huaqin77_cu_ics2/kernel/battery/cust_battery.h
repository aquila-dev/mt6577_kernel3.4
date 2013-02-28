#ifndef _CUST_BAT_H_
#define _CUST_BAT_H_

typedef enum
{
	Cust_CC_1600MA = 0x0,
	Cust_CC_1500MA = 0x1,
	Cust_CC_1400MA = 0x2,
	Cust_CC_1300MA = 0x3,
	Cust_CC_1200MA = 0x4,
	Cust_CC_1100MA = 0x5,
	Cust_CC_1000MA = 0x6,
	Cust_CC_900MA  = 0x7,
	Cust_CC_800MA  = 0x8,
	Cust_CC_700MA  = 0x9,
	Cust_CC_650MA  = 0xA,
	Cust_CC_550MA  = 0xB,
	Cust_CC_450MA  = 0xC,
	Cust_CC_400MA  = 0xD,
	Cust_CC_200MA  = 0xE,
	Cust_CC_70MA   = 0xF,
	Cust_CC_0MA	   = 0xDD
}cust_charging_current_enum;

typedef struct{	
	unsigned int BattVolt;
	unsigned int BattPercent;
}VBAT_TO_PERCENT;

/* Battery Temperature Protection */
#if defined(HQ_CHARGER_FOR_HUAWEI)
#define MAX_CHARGE_TEMPERATURE  51
#define MIN_CHARGE_TEMPERATURE  -11
#else
#define MAX_CHARGE_TEMPERATURE  50
#define MIN_CHARGE_TEMPERATURE  0
#endif
#define ERR_CHARGE_TEMPERATURE  0xFF

/* Charging Current Setting */
#define CONFIG_USB_IF 						0   
#define USB_CHARGER_CURRENT_SUSPEND			Cust_CC_0MA		// def CONFIG_USB_IF
#define USB_CHARGER_CURRENT_UNCONFIGURED	Cust_CC_70MA	// def CONFIG_USB_IF
#define USB_CHARGER_CURRENT_CONFIGURED		Cust_CC_450MA	// def CONFIG_USB_IF
#define USB_CHARGER_CURRENT					Cust_CC_450MA
//kaka_11_1110_3 add
#if defined(MTK_CTA_SUPPORT) || defined (HQ_DUAL_STANDBY_CTA_SUPPORT) //kaka_11_1107
#define AC_CHARGER_CURRENT					Cust_CC_450MA	//kaka_12_0102
#else
#define AC_CHARGER_CURRENT		Cust_CC_800MA    //kaka_12_0102
#endif
//kaka_11_1110_3 end

#if defined(HQ_CHARGER_FOR_HUAWEI)
//hupeng 120120,the charger current is different for ac and usb in hi or low temperature.
#define AC_CHARGER_CURRENT_HI_TEMP		Cust_CC_800MA
#define AC_CHARGER_CURRENT_LOW_TEMP		Cust_CC_200MA
#define USB_CHARGER_CURRENT_HI_TEMP		Cust_CC_450MA
#define USB_CHARGER_CURRENT_LOW_TEMP	Cust_CC_200MA
/* Recharging Battery Voltage */
#define RECHARGING_VOLTAGE_HI_TEMP      4110
#define RECHARGING_VOLTAGE_LOW_TEMP      3800
#else
/* Recharging Battery Voltage */
#define RECHARGING_VOLTAGE      4110
#endif

/* Battery Meter Solution */
#define CONFIG_ADC_SOLUTION 	1

/* Battery Voltage and Percentage Mapping Table */
VBAT_TO_PERCENT Batt_VoltToPercent_Table[] = {
	/*BattVolt,BattPercent*/
#if defined(HQ_CHARGER_FOR_HUAWEI)
	{3400,0},	//hupeng 120121,the power off volt is 3.5v for huawei,120309,modify to 3.4v
#else
	{3400,0},
#endif
	{3641,10},
	{3708,20},
	{3741,30},
	{3765,40},
	{3793,50},
	{3836,60},
	{3891,70},
	{3960,80},
	{4044,90},
	{4183,100},
};

/* Precise Tunning */
//#define BATTERY_AVERAGE_SIZE 	30
#define BATTERY_AVERAGE_SIZE   12

/* Common setting */
#define R_CURRENT_SENSE 2				// 0.2 Ohm
#define R_BAT_SENSE 4					// times of voltage
#define R_I_SENSE 4						// times of voltage
#define R_CHARGER_1 330
#define R_CHARGER_2 39
#define R_CHARGER_SENSE ((R_CHARGER_1+R_CHARGER_2)/R_CHARGER_2)	// times of voltage

#if defined(MTK_CTA_SUPPORT) || defined (HQ_DUAL_STANDBY_CTA_SUPPORT) 
#define V_CHARGER_MAX 6200				// kaka_11_1208 CTA want 6.2v  
#else
#define V_CHARGER_MAX 6000				// hupeng 120120, huawei want 6.0v  //#if defined(HQ_CHARGER_FOR_HUAWEI)
#endif

#define V_CHARGER_MIN 4400				// 4.4 V
#define V_CHARGER_ENABLE 0				// 1:ON , 0:OFF

/* Teperature related setting */
#define RBAT_PULL_UP_R             24000
//#define RBAT_PULL_UP_VOLT          2500
#define RBAT_PULL_UP_VOLT          1200
#define TBAT_OVER_CRITICAL_LOW     68237
//#define TBAT_OVER_CRITICAL_LOW     483954
#if defined(HQ_CHARGER_FOR_HUAWEI)
#define BAT_TEMP_PROTECT_ENABLE    1
#else
#define BAT_TEMP_PROTECT_ENABLE    0
#endif
#define BAT_NTC_10 1	//kaka_12_1230 Huawei use 10K 
#define BAT_NTC_47 0
//#define BAT_NTC_TSM_1

/* Battery Notify */
#define BATTERY_NOTIFY_CASE_0001
#define BATTERY_NOTIFY_CASE_0002
#ifndef HQ_CHARGER_FOR_HUAWEI
#define BATTERY_NOTIFY_CASE_0003
#endif
#define BATTERY_NOTIFY_CASE_0004
#define BATTERY_NOTIFY_CASE_0005

#if defined(MTK_CTA_SUPPORT) || defined (HQ_DUAL_STANDBY_CTA_SUPPORT) //kaka_11_1107
//kaka_11_0601 add
#define BATTERY_NOTIFY_CASE_SOFT_OVP
#define BATTERY_NOTIFY_CASE_HARD_OVP
#define BATTERY_NOTIFY_CASE_BATTERY_FULL
//kaka_11_0601 end
#endif
#endif /* _CUST_BAT_H_ */ 
