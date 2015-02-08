#include <linux/types.h>
#include <mach/mt6577_pm_ldo.h>
#include <cust_alsps.h>
//#include <mach/mt6577_pm_ldo.h>

static struct alsps_hw cust_alsps_hw = {
    .i2c_num    = 0,
	.polling_mode_ps =1,
	.polling_mode_als =1,
    .power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
    .i2c_addr   = {0x72, 0x48, 0x78, 0x00},
    /*Lenovo-sw chenlj2 add 2011-06-03,modify parameter below two lines*/
    .als_level  = { 2,  4,  8, 15,  50, 100, 200, 400,  800, 1600, 2000, 2500,  3500,  5000, 10000, 65535},
	.als_value  = {10, 20, 40, 90, 160, 225, 320, 640, 1280, 2600, 3500, 9000, 11000, 20000, 30240, 30240},
    .ps_threshold_high = 650,
    .ps_threshold_low = 600,
    .ps_threshold = 900,
};
struct alsps_hw *get_cust_alsps_hw(void) {
    return &cust_alsps_hw;
}
int TMD2771_CMM_PPCOUNT_VALUE = 0x0A;
int ZOOM_TIME = 4;
int TMD2771_CMM_CONTROL_VALUE = 0x20;
