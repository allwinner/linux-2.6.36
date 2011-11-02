
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

#define mmc_pwr_msg(...)    do {printk("[mmc_pwr]: "__VA_ARGS__);} while(0)
u32 mmc_pio_hdle;

static unsigned int sdio_card_used;
static unsigned int sdio_cardid;
static unsigned int module_sel;

extern void nano_wifi_gpio_init(void);
extern void usi_bm01a_gpio_init(void);
extern void apm_6xxx_gpio_init(void);
extern void swbb23_gpio_init(void);

unsigned int get_sdio_wifi_module_select(void)
{
    return module_sel;
}
EXPORT_SYMBOL(get_sdio_wifi_module_select);

static int __init swmmc_power_init(void)
{
    int ret;
    char* wifi_para = "sdio_wifi_para";
    char* wifi_mod[] = {" ", "swl-n20", "usi-bm01a", "ar6302qfn", "apm6xxx", "swb-b23"};
    
    ret = script_parser_fetch(wifi_para, "sdio_wifi_used", &sdio_card_used, sizeof(unsigned)); 
    if (ret) {
        mmc_pwr_msg("failed to fetch sdio card configuration!\n");
        return 0;
    }
    if (!sdio_card_used) {
        mmc_pwr_msg("no sdio card used in configuration\n");
        return 0;
    }
    
    ret = script_parser_fetch(wifi_para, "sdio_wifi_sdc_id", &sdio_cardid, sizeof(unsigned));
    if (ret) {
        mmc_pwr_msg("failed to fetch sdio card's sdcid\n");
        return 0;
    }

    ret = script_parser_fetch(wifi_para, "sdio_wifi_mod_sel", &module_sel, sizeof(unsigned));
    if (ret) {
        mmc_pwr_msg("failed to fetch sdio module select\n");
        return 0;
    }
    printk("[wifi]: Select sdio wifi: %s !!\n", wifi_mod[module_sel]);
    
    mmc_pio_hdle = gpio_request_ex(wifi_para, NULL);
    if (!mmc_pio_hdle) {
        mmc_pwr_msg("failed to fetch sdio card's io handler, please check it !!\n");
        return 0;
    }
    
    switch (module_sel) {
        case 1: /* nano wifi */
            nano_wifi_gpio_init();
            break;
        case 2: /* usi bm01a */
            usi_bm01a_gpio_init();
            break;
        case 3: /* ar6302qfn */
            
            break;
        case 4: /* apm 6xxx */
            apm_6xxx_gpio_init();
            break;
        case 5: /* swb b23 */
            swbb23_gpio_init();
            break;
        default:
            mmc_pwr_msg("Wrong sdio module select %d !!\n", module_sel);
    }
    
    mmc_pwr_msg("SDIO card gpio reource is OK !!\n");
    
    return 0;
}

static void __exit swmmc_power_exit(void)
{
    switch (module_sel) {
        case 1: /* nano wifi */
            nano_wifi_gpio_init();
            break;
        case 2: /* usi bm01a */
            usi_bm01a_gpio_init();
            break;
        case 3: /* ar6302qfn */
            
            break;
        case 4: /* usi bm01a */
            apm_6xxx_gpio_init();
        case 5: /* swb b23 */
            swbb23_gpio_init();
            break;
        default:
            mmc_pwr_msg("Wrong sdio module select %d !!\n", module_sel);
    }
    
    if (mmc_pio_hdle)
        gpio_release(mmc_pio_hdle, 2);
    
    mmc_pio_hdle = 0;
    sdio_card_used = 0;
    sdio_cardid = 0;
    module_sel = 0;
    mmc_pwr_msg("SDIO card resource is released !!\n");
}

module_init(swmmc_power_init);
module_exit(swmmc_power_exit);

