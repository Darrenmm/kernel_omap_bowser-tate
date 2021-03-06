/*
 * Backlight driver for bowser board, based on TI & O2Micro chips
 *
 * Copyright (C) Amazon Technologies Inc. All rights reserved.  
 * Feng Ye (fengy@lab126.com)  
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <plat/board.h>
#include <plat/dmtimer.h>
#include <plat/board-backlight.h>
#if 0
#include <linux/earlysuspend.h>
#endif

#define TIMERVALUE_OVERFLOW    0xFFFFFFFF
#define TIMERVALUE_RELOAD      (TIMERVALUE_OVERFLOW - ctx->timer_range) /* the reload value when timer overflows, the bigger the shorter the whole duty cycle */


struct bowser_backlight_ctx {
	struct work_struct    work;
	struct omap_dm_timer *timer;
	int gpio_en_o2m;
	int gpio_vol_o2m;
	int gpio_cabc_en;
	unsigned long timer_range;
	unsigned int brightness_max;
	unsigned int brightness_set; /* brightness will be set to */
	unsigned int brightness;     /* current brightness */
#if 0
	unsigned int brightness_saved; /* saved brightness for early suspend */
	struct early_suspend early_suspend;
#endif
};

static void bowser_backlight_set_brightness(struct bowser_backlight_ctx *ctx)
{
	unsigned long cmp_value;

	printk("brightness_set %x\n", ctx->brightness_set);
/* JossCheng, 20111220, remove this temporary fix { */	
#if 0
	/* Temporary fix */
	if(ctx->brightness_set == 0)
	{
		printk("Setting it to 80 for now\n");
		ctx->brightness_set = 80;
	}
#endif 
/* JossCheng, 20111220, remove this temporary fix } */

	/* Set the new brightness */
	if (ctx->brightness_set == ctx->brightness)
		return;

	if (ctx->brightness_set == ctx->brightness_max) {
		cmp_value = TIMERVALUE_OVERFLOW+1;
	}
	else {
		cmp_value = (ctx->timer_range * ctx->brightness_set)/ctx->brightness_max + TIMERVALUE_RELOAD;
	}
	printk("Bowser backlight cmp val=%lx\n", cmp_value);

	if (ctx->brightness_set) {
		omap_dm_timer_enable(ctx->timer);
		omap_dm_timer_set_source(ctx->timer,  OMAP_TIMER_SRC_SYS_CLK);
		omap_dm_timer_set_prescaler(ctx->timer, 0);

		omap_dm_timer_start(ctx->timer);
		omap_dm_timer_set_match(ctx->timer, 1 /* compare enable */, cmp_value);
		omap_dm_timer_set_load(ctx->timer, 1, TIMERVALUE_RELOAD);
		omap_dm_timer_set_pwm(ctx->timer, 0 /* positive pulse */, 1 /* toggle */, OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE /* trigger */);
		/* Enabling it as it's disabled after setting pwm */
		omap_dm_timer_enable(ctx->timer);
	}
	else {
		omap_dm_timer_stop(ctx->timer);
		omap_dm_timer_disable(ctx->timer);
	}

	ctx->brightness = ctx->brightness_set;
}


static void bowser_backlight_work(struct work_struct *work)
{
	struct bowser_backlight_ctx *ctx = container_of(work, struct bowser_backlight_ctx, work);
	int power_up   = !ctx->brightness && ctx->brightness_set;
	int power_down = ctx->brightness && !ctx->brightness_set;
	int ret = 0;

/* JossCheng, 20111220, remove this temporary fix { */	
#if 0
	/* Temporary fix */
	power_up = 1;
#endif
/* JossCheng, 20111220, remove this temporary fix } */	

	if (power_up) {
		printk("backlight powering up\n");

		/* Enable VIN */
		if (ctx->gpio_vol_o2m >= 0) {
			gpio_set_value(ctx->gpio_vol_o2m, 1);
		}

		/* Enable PWM */
		bowser_backlight_set_brightness(ctx);

		/* Enable the O2M */
		if (ctx->gpio_en_o2m >= 0) {
			gpio_set_value(ctx->gpio_en_o2m, 1);
		}
	}
	else if (power_down) {
		printk("backlight powering down\n");

		/* Disable the O2M */
		if (ctx->gpio_en_o2m >= 0) {
			gpio_set_value(ctx->gpio_en_o2m, 0);
		}

		/* Disable PWM */
		bowser_backlight_set_brightness(ctx);

		/* Disable VIN */
		if (ctx->gpio_vol_o2m >= 0) {
			gpio_set_value(ctx->gpio_vol_o2m, 0);
		}

	}
	else {
		bowser_backlight_set_brightness(ctx);
	}

}

/* this function will be called during suspend and resume */
static int bowser_backlight_update_status(struct backlight_device *bl)
{
	struct bowser_backlight_ctx *ctx = bl_get_data(bl);
	int brightness = bl->props.brightness;

/* JossCheng, 20111220, remove these condiction checks: props.power, fb_blank and state are not initial { */
#if 0
  printk("Joss: power=%d, fb_blank=%d, state=%d\n",bl->props.power,bl->props.fb_blank,bl->props.state);
	if (bl->props.power != FB_BLANK_UNBLANK)
		brightness = 0;

	if (bl->props.fb_blank != FB_BLANK_UNBLANK)
		brightness = 0;
#endif
	if (bl->props.state & (BL_CORE_SUSPENDED | BL_CORE_FBBLANK))
		brightness = 0;
/* JossCheng, 20111220, remove these condiction checks: props.power, fb_blank and state are not initial } */

	ctx->brightness_set = brightness;
	schedule_work(&ctx->work);

	return 0;
}

static int bowser_backlight_get_brightness(struct backlight_device *bl)
{
	struct bowser_backlight_ctx *ctx = bl_get_data(bl);
	return ctx->brightness;
}

static const struct backlight_ops bowser_backlight_ops = {
	.options = BL_CORE_SUSPENDRESUME,
	.update_status	= bowser_backlight_update_status,
	.get_brightness	= bowser_backlight_get_brightness,
};

#if 0
static void bowser_backlight_early_suspend(struct early_suspend *h)
{
	struct bowser_backlight_ctx *ctx = container_of(h, struct bowser_backlight_ctx, early_suspend);

	ctx->brightness_saved = ctx->brightness;
	ctx->brightness_set = 0;
	schedule_work(&ctx->work);
}

static void bowser_backlight_late_resume(struct early_suspend *h)
{
	struct bowser_backlight_ctx *ctx = container_of(h, struct bowser_backlight_ctx, early_suspend);

	ctx->brightness_set = ctx->brightness_saved;
	schedule_work(&ctx->work);
}
#endif


static int bowser_backlight_probe(struct platform_device *pdev)
{
	struct bowser_backlight_ctx *ctx;
	struct backlight_device *bl;
	struct backlight_properties props;
	struct bowser_backlight_platform_data *pdata = pdev->dev.platform_data;
	int    ret = 0;


	ctx = kzalloc(sizeof(struct bowser_backlight_ctx), GFP_KERNEL);
	if (ctx == NULL) {
		dev_err(&pdev->dev, "No memory for backlight device\n");
		return -ENOMEM;
	}

	/* timer request */
	ctx->timer = omap_dm_timer_request_specific(pdata->timer);
	if (ctx->timer == NULL) {
		kfree(ctx);
		dev_err(&pdev->dev, "failed to request pwm timer\n");
		return -ENODEV;
	}

	/* gpio request */
	ctx->gpio_en_o2m = pdata->gpio_en_o2m;
	if (pdata->gpio_en_o2m >= 0){
		ret = gpio_request(ctx->gpio_en_o2m, "backlight_o2m_gpio");
		if (ret != 0) {  /* this GPIO is shared with HDMI so it could fail */
			printk("backlight gpio request failed\n");
			ctx->gpio_en_o2m = -EINVAL; /* serve as a flag as well */
		}
	}
	gpio_direction_output(ctx->gpio_en_o2m, 1);

	ctx->gpio_vol_o2m = pdata->gpio_vol_o2m;
	if (pdata->gpio_vol_o2m >= 0){
		ret = gpio_request(ctx->gpio_vol_o2m, "backlight_vol_gpio");
		if (ret != 0) {
			printk("backlight vol gpio request failed\n");
			ctx->gpio_vol_o2m = -EINVAL; /* serve as a flag as well */
		}
		gpio_direction_output(ctx->gpio_vol_o2m, 1);
	}

       ctx->gpio_cabc_en = pdata->gpio_cabc_en;
       if (pdata->gpio_cabc_en > 0){
               ret = gpio_request(ctx->gpio_cabc_en, "backlight_cabc_gpio");
               if (ret != 0) {
                       printk("backlight cabc gpio request failed\n");
                       ctx->gpio_cabc_en = 0; /* serve as a flag as well */
               }
               else {
                       gpio_direction_output(ctx->gpio_cabc_en, 1);
                       gpio_set_value(ctx->gpio_cabc_en, 1); /* For CABC we enable it right away */
               }
       }


	/* timer count range value calculate, the bigger the longer the whole duty cycle */
	/* *2 because PWM high and low count as one cycle */
	ctx->timer_range = (pdata->sysclk / (pdata->pwmfreq * 2));

	/* backlight device register */
	ctx->brightness_max  = pdata->totalsteps;
	props.max_brightness = pdata->totalsteps;
	props.type = BACKLIGHT_RAW;
	bl = backlight_device_register("bowser", &pdev->dev, ctx, &bowser_backlight_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight device\n");
		kfree(ctx);
		return PTR_ERR(bl);
	}

#if 0
	ctx->early_suspend.level   = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ctx->early_suspend.suspend = bowser_backlight_early_suspend;
	ctx->early_suspend.resume  = bowser_backlight_late_resume;
	register_early_suspend(&ctx->early_suspend);
#endif

	INIT_WORK(&ctx->work, bowser_backlight_work);

	bl->props.brightness = pdata->initialstep; /* do we want to save the value user set previously? */

	platform_set_drvdata(pdev, bl);

	//backlight_update_status(bl);

	return 0;
}

static int bowser_backlight_remove(struct platform_device *pdev)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct bowser_backlight_ctx *ctx = bl_get_data(bl);

	backlight_device_unregister(bl);
	kfree(ctx);
	return 0;
}

static struct platform_driver bowser_backlight_driver = {
	.driver		= {
		.name	= "backlight",
		.owner	= THIS_MODULE,
	},
	.probe		= bowser_backlight_probe,
	.remove		= bowser_backlight_remove,
};

static int __init bowser_backlight_init(void)
{
	return platform_driver_register(&bowser_backlight_driver);
}
module_init(bowser_backlight_init);

static void __exit bowser_backlight_exit(void)
{
	platform_driver_unregister(&bowser_backlight_driver);
}
module_exit(bowser_backlight_exit);

MODULE_DESCRIPTION("Backlight Driver for Bowser");
MODULE_AUTHOR("Feng Ye <fengy@lab126.com");
MODULE_LICENSE("GPL");
