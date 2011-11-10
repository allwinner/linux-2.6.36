#ifndef MMC_PM_H
#define MMC_PM_H

#define SDIO_WIFI_POWERUP   (1)
#define SDIO_WIFI_INSUSPEND (2)

struct mmc_pm_ops {
    char*   mod_name;
    u32     sdio_card_used;
    u32     sdio_cardid;
    u32     module_sel;
    u32     pio_hdle;
    int     (*gpio_ctrl)(char* name, int level);
    int     (*get_io_val)(char* name);
    void    (*standby)(int in);
};

void mmc_pm_ops_register(struct mmc_pm_ops* ops);
void mmc_pm_ops_register(struct mmc_pm_ops* ops);

void nano_wifi_gpio_init(void);
void usi_bm01a_gpio_init(void);
void apm_6xxx_gpio_init(void);
void swbb23_gpio_init(void);

extern struct mmc_pm_ops mmc_card_pm_ops;
extern void sw_mmc_rescan_card(unsigned id, unsigned insert);

#endif
