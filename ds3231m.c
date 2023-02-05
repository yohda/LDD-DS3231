#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/kernel.h>

/* mdelay() */
#include <linux/delay.h>

#include <linux/rtc.h>

#define DS3231M_SECONDS_REG_ADDR			(0x00)
#define DS3231M_MINUTES_REG_ADDR			(0x01)
#define DS3231M_HOURS_REG_ADDR				(0x02)
	#define DS3231M_HOURS_12_AM_PM			(0x40)
	#define DS3231M_HOURS_24				(0x00)

#define DS3231M_DAY_REG_ADDR				(0x03)
#define DS3231M_DATE_REG_ADDR				(0x04)
#define DS3231M_MONTH_CENTURY_REG_ADDR		(0x05)
#define DS3231M_YEAR_REG_ADDR				(0x06)

#define DS3231M_CONTROL_REG_ADDR			(0xE0)
#define DS3231M_STATUS_REG_ADDR 			(0xF0)

#define DS3231M_NAME "ds3231m"

enum {
	MONDAY = 1,
	TUESDAY,
	WEDNESDAY,
	THURSDAY,
	FRIDAY,
	SUARDAY,
	SATURDAY,
	SUNDAY,
	MAX_DAY,
};

/* below properties of ds3231m_status are decimal format, not bcd */
struct ds3231m_status {
	u8 seconds;
	u8 minutes;
	u8 hours;
	u8 day;
	u8 date;
};

struct ds3231m_dev {
	struct i2c_client *client;
	struct ds3231m_status status;
};

static int ds3231m_i2c_write(const struct i2c_client *client, const u8 reg, const u8 data)
{
	if(!client)
	{
		pr_err("i2c istances isn`t exist.\n");
		return -EINVAL;
	}

	if(i2c_smbus_write_byte_data(client, reg, data) < 0)
	{
		pr_err("bmp180 i2c write error. reg:0x%x, data:0x%x\n", reg, data);
		return -EIO;
	}
     
    return 0;
}

static int ds3231m_i2c_read(const struct i2c_client *client, const u8 reg, u8 *buf)
{
	int ret = 0;
	if(!client)
	{
		pr_err("i2c istances isn`t exist.\n");
		return -EINVAL;
	}
	
	ret = i2c_smbus_read_byte_data(client, reg);
	if(ret < 0)
	{
		pr_err("bmp180 i2c read error. reg:0x%x\n", reg);
		return -EIO;
	}

	*buf = (u8)(ret & 0xFF);
	 
    return 0;
}

static u8 ds3231m_util_bcd2dec(const u8 bcd)
{
	return (bcd >> 4) * 10 + (bcd % 10);
}

static u8 ds3231m_util_dec2bcd(const u8 dec)
{
	return (dec / 10) << 4 + (dec % 16);
}

static int ds3231m_set_seconds(struct ds3231m_dev* ds3231m, const u8 seconds)
{
	int ret = 0;
	u8 bcd_seconds;
	
	if(seconds < 0 || seconds > 59) {
		return -EINVAL;
	}

	bcd_seconds = ds3231m_util_dec2bcd(seconds);
	ret = ds3231m_i2c_write(ds3231m->client, DS3231M_SECONDS_REG_ADDR, &bcd_seconds);
	if(ret) {
		return ret;
	}

	return 0;
}

static int ds3231m_set_minutes(struct ds3231m_dev* ds3231m, const u8 minutes)
{
	int ret = 0;
	u8 bcd_minutes;
	
	if(minutes < 0 || minutes > 59) {
		return -EINVAL;
	}

	bcd_minutes = ds3231m_util_dec2bcd(minutes);
	ret = ds3231m_i2c_write(ds3231m->client, DS3231M_MINUTES_REG_ADDR, &bcd_minutes);
	if(ret) {
		return ret;
	}

	return 0;
}

static int ds3231m_set_hours(struct ds3231m_dev* ds3231m, const u8 hours)
{
	int ret = 0;
	u8 bcd_hours;

	if(hours < 0)
		return -EINVAL;

	if((ds3231m->status.hours) & DS3231M_HOURS_12_AM_PM) {
		if(hours > 12) {
			return -EINVAL;
		}
	} else {
		if(hours > 24) {
			return - EINVAL;	
		}
	}

	bcd_hours = ds3231m_util_dec2bcd(hours);
	ret = ds3231m_i2c_write(ds3231m->client, DS3231M_HOURS_REG_ADDR, &bcd_hours);
	if(ret) {
		return ret;
	}

	return 0;
}

static int ds3231m_set_day(struct ds3231m_dev* ds3231m, const u8 day)
{
	int ret = 0;
	u8 bcd_days;
	
	if(day < MONDAY || day > SUNDAY) {
		return -EINVAL;
	}

	bcd_days = ds3231m_util_dec2bcd(day);
	ret = ds3231m_i2c_write(ds3231m->client, DS3231M_DAY_REG_ADDR, &bcd_days);
	if(ret) {
		return ret;
	}
}

static int ds3231m_set_date(struct ds3231m_dev* ds3231m, const u8 date)
{
	int ret = 0;
	u8 bcd_date;
	
	if(date < 1 || date > 31) {
		return -EINVAL;
	}

	bcd_date = ds3231m_util_dec2bcd(date);
	ret = ds3231m_i2c_write(ds3231m->client, DS3231M_DATE_REG_ADDR, &bcd_date);
	if(ret) {
		return ret;
	}
}

static int ds3231m_set_month(struct ds3231m_dev* ds3231m, const u8 month)
{
	int ret = 0;
	u8 bcd_month;
	
	if(month < 1 || month > 12) {
		return -EINVAL;
	}

	bcd_month = ds3231m_util_dec2bcd(month);
	ret = ds3231m_i2c_write(ds3231m->client, DS3231M_MONTH_CENTURY_REG_ADDR, &bcd_month);
	if(ret) {
		return ret;
	}
}

static int ds3231m_set_year(struct ds3231m_dev* ds3231m)
{
	return 0;
}

static int ds3231m_parse_dt(void)
{
	return 0;
}

static int ds3231m_init(const struct i2c_client *client)
{
	int ret = 0;

	return 0;	
}


static int ds3231m_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    int err = 0, i = 100;
	struct ds3231m_dev *ds3231m;
	u8 u = 0x00;

	pr_info("ds3231m Probe Started!!!\n");

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
	{
		pr_err("No support i2c smbus");	
		return -EIO;
	}

	ds3231m = devm_kzalloc(&client->dev, sizeof(*ds3231m), GFP_KERNEL);
	if(!ds3231m)
	{
		pr_err("Couldn`t allocate the memory.\n");
		return -ENOMEM;
	}

	while(i--)
	{
		err = ds3231m_i2c_read(client, DS3231M_SECONDS_REG_ADDR, &u);
		pr_info("0x00:0x%x\n", u);
		err = ds3231m_i2c_read(client, DS3231M_MINUTES_REG_ADDR, &u);
		pr_info("0x01:0x%x\n", u);
		err = ds3231m_i2c_read(client, DS3231M_HOURS_REG_ADDR, &u);
		pr_info("0x02:0x%x\n", u);
		err = ds3231m_i2c_read(client, DS3231M_DAY_REG_ADDR, &u);
		pr_info("0x03:0x%x\n", u);
		err = ds3231m_i2c_read(client, DS3231M_DATE_REG_ADDR, &u);
		pr_info("0x03:0x%x\n", u);

		msleep(500);
	}
	
	
	pr_info("ds3231m Probe Done!!!\n");   	
	return 0;
}

static int ds3231m_remove(struct i2c_client *client)
{   
    pr_info("ds3231m Removed!!!\n");
    return 0;
}

static const struct of_device_id ds3231m_dts[] = {
        { .compatible = "maxim,ds3231m" },
        {}
};
MODULE_DEVICE_TABLE(of, ds3231m_dts);

static struct i2c_driver ds3231m_driver = {
        .driver = {
            .name   = DS3231M_NAME,
            .owner  = THIS_MODULE,
			.of_match_table = ds3231m_dts,
        },
        .probe          = ds3231m_probe,
        .remove         = ds3231m_remove,
};

module_i2c_driver(ds3231m_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yohan Yoon <dbsdy1235@gmail.com>");
MODULE_DESCRIPTION("BMP180 Device Driver");
